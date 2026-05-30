#pragma once

#include <JuceHeader.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace elaw
{
    struct EditorBindings
    {
        juce::StringArray sliders;
        juce::StringArray combos;
        juce::StringArray toggles;
        juce::String webViewDataFolderName;
        juce::Colour background;
    };

    inline juce::String stripQueryAndFragment (juce::String path)
    {
        path = path.upToFirstOccurrenceOf ("?", false, false);
        path = path.upToFirstOccurrenceOf ("#", false, false);
        return path;
    }

    inline juce::File getSiblingWebUiDistDirectory (const char* sourceFile)
    {
        return juce::File::createFileWithoutCheckingPath (sourceFile)
            .getParentDirectory()
            .getParentDirectory()
            .getChildFile ("web-ui")
            .getChildFile ("dist");
    }

    inline juce::File getBundledWebUiDistDirectory()
    {
        const auto executable = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
        return executable
            .getParentDirectory()
            .getParentDirectory()
            .getChildFile ("Resources")
            .getChildFile ("web-ui");
    }

    class WebBrowser final : public juce::WebBrowserComponent
    {
    public:
        using juce::WebBrowserComponent::WebBrowserComponent;

        bool pageAboutToLoad (const juce::String& newURL) override
        {
            const auto& root = juce::WebBrowserComponent::getResourceProviderRoot();
            return newURL == root || newURL.startsWith (root);
        }
    };

    class WebViewEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
    {
    public:
        using LevelProvider = std::function<std::pair<float, float>()>;
        using WebResource = juce::WebBrowserComponent::Resource;

        WebViewEditor (juce::AudioProcessor& processor,
                       juce::AudioProcessorValueTreeState& valueTreeState,
                       EditorBindings bindingsToUse,
                       juce::File webRoot,
                       LevelProvider provider)
            : AudioProcessorEditor (&processor),
              parameters (valueTreeState),
              bindings (std::move (bindingsToUse)),
              webUiDistDirectory (getBundledWebUiDistDirectory().isDirectory() ? getBundledWebUiDistDirectory()
                                                                               : std::move (webRoot)),
              levelProvider (std::move (provider))
        {
            for (const auto& id : bindings.sliders)
            {
                auto relay = std::make_unique<juce::WebSliderRelay> (id);
                sliderAttachments.push_back (std::make_unique<juce::WebSliderParameterAttachment> (parameter (id), *relay));
                sliderRelays.push_back (std::move (relay));
            }

            for (const auto& id : bindings.combos)
            {
                auto relay = std::make_unique<juce::WebComboBoxRelay> (id);
                comboAttachments.push_back (std::make_unique<juce::WebComboBoxParameterAttachment> (parameter (id), *relay));
                comboRelays.push_back (std::move (relay));
            }

            for (const auto& id : bindings.toggles)
            {
                auto relay = std::make_unique<juce::WebToggleButtonRelay> (id);
                toggleAttachments.push_back (std::make_unique<juce::WebToggleButtonParameterAttachment> (parameter (id), *relay));
                toggleRelays.push_back (std::move (relay));
            }

            webComponent = std::make_unique<WebBrowser> (createWebOptions());
            addAndMakeVisible (*webComponent);
            webComponent->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

            setResizable (false, false);
            setSize (900, 620);
            startTimerHz (30);
        }

        ~WebViewEditor() override
        {
            stopTimer();
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (bindings.background);
        }

        void resized() override
        {
            if (webComponent != nullptr)
                webComponent->setBounds (getLocalBounds());
        }

        int getControlParameterIndex (juce::Component&) override
        {
            return controlParameterIndexReceiver.getControlParameterIndex();
        }

    private:
        juce::WebBrowserComponent::Options createWebOptions()
        {
            auto options = juce::WebBrowserComponent::Options {};

           #if JUCE_WINDOWS
            options = options.withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                             .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2 {}
                                 .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                                          .getChildFile (bindings.webViewDataFolderName))
                                 .withStatusBarDisabled()
                                 .withBuiltInErrorPageDisabled()
                                 .withBackgroundColour (bindings.background));
           #endif

            options = options.withNativeIntegrationEnabled();

            for (auto& relay : sliderRelays)
                options = options.withOptionsFrom (*relay);

            for (auto& relay : comboRelays)
                options = options.withOptionsFrom (*relay);

            for (auto& relay : toggleRelays)
                options = options.withOptionsFrom (*relay);

            return options.withOptionsFrom (controlParameterIndexReceiver)
                          .withResourceProvider ([this] (const auto& url)
                                                 {
                                                     return getResource (url);
                                                 });
        }

        std::optional<WebResource> getResource (const juce::String& url) const
        {
            auto requestPath = url == "/" ? juce::String { "index.html" }
                                          : url.fromFirstOccurrenceOf ("/", false, false);

            requestPath = stripQueryAndFragment (requestPath);

            if (requestPath.isEmpty())
                requestPath = "index.html";

            if (requestPath.contains ("..") || requestPath.startsWithChar ('/'))
                return std::nullopt;

            auto file = webUiDistDirectory.getChildFile (requestPath);

            if (! file.existsAsFile())
                return std::nullopt;

            return WebResource { readFileAsBytes (file),
                                 juce::String { getMimeForExtension (file.getFileExtension().fromFirstOccurrenceOf (".", false, false)) } };
        }

        void timerCallback() override
        {
            if (webComponent == nullptr || ! webComponent->isVisible() || ! levelProvider)
                return;

            const auto levels = levelProvider();
            juce::DynamicObject::Ptr payload { new juce::DynamicObject() };
            payload->setProperty ("input", levels.first);
            payload->setProperty ("output", levels.second);
            webComponent->emitEventIfBrowserIsVisible ("levels", juce::var { payload.get() });
        }

        static const char* getMimeForExtension (const juce::String& extension)
        {
            const auto ext = extension.toLowerCase();

            if (ext == "html" || ext == "htm") return "text/html";
            if (ext == "js" || ext == "mjs")   return "text/javascript";
            if (ext == "css")                  return "text/css";
            if (ext == "json" || ext == "map") return "application/json";
            if (ext == "svg")                  return "image/svg+xml";
            if (ext == "png")                  return "image/png";
            if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
            if (ext == "ico")                  return "image/vnd.microsoft.icon";
            if (ext == "woff2")                return "font/woff2";

            return "application/octet-stream";
        }

        static std::vector<std::byte> readFileAsBytes (const juce::File& file)
        {
            juce::FileInputStream stream { file };

            if (! stream.openedOk())
                return {};

            std::vector<std::byte> data (static_cast<size_t> (stream.getTotalLength()));
            stream.read (data.data(), static_cast<int> (data.size()));
            return data;
        }

        juce::RangedAudioParameter& parameter (const juce::String& parameterID) const
        {
            auto* value = parameters.getParameter (parameterID);
            jassert (value != nullptr);
            return *value;
        }

        juce::AudioProcessorValueTreeState& parameters;
        EditorBindings bindings;
        juce::File webUiDistDirectory;
        LevelProvider levelProvider;

        std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;
        std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;
        std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;
        juce::WebControlParameterIndexReceiver controlParameterIndexReceiver;

        std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
        std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;
        std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

        std::unique_ptr<WebBrowser> webComponent;
    };
}
