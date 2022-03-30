// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_

#include <memory>
#include <set>

#include "base/callback.h"
#include "base/i18n/rtl.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/types/pass_key.h"
#include "base/unguessable_token.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom-shared.h"
#include "third_party/blink/public/common/context_menu_data/untrustworthy_context_menu_params.h"
#include "third_party/blink/public/common/css/page_size_type.h"
#include "third_party/blink/public/common/feature_policy/feature_policy_features.h"
#include "third_party/blink/public/common/frame/user_activation_update_source.h"
#include "third_party/blink/public/common/messaging/transferable_message.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "third_party/blink/public/mojom/ad_tagging/ad_frame.mojom-shared.h"
#include "third_party/blink/public/mojom/blob/blob_url_store.mojom-shared.h"
#include "third_party/blink/public/mojom/commit_result/commit_result.mojom-shared.h"
#include "third_party/blink/public/mojom/devtools/devtools_agent.mojom-shared.h"
#include "third_party/blink/public/mojom/devtools/inspector_issue.mojom-shared.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy.mojom-shared.h"
#include "third_party/blink/public/mojom/frame/lifecycle.mojom-shared.h"
#include "third_party/blink/public/mojom/frame/media_player_action.mojom-shared.h"
#include "third_party/blink/public/mojom/frame/user_activation_notification_type.mojom-shared.h"
#include "third_party/blink/public/mojom/page/widget.mojom-shared.h"
#include "third_party/blink/public/mojom/portal/portal.mojom-shared.h"
#include "third_party/blink/public/mojom/selection_menu/selection_menu_behavior.mojom-shared.h"
#include "third_party/blink/public/mojom/web_feature/web_feature.mojom-shared.h"
#include "third_party/blink/public/platform/cross_variant_mojo_util.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_document_loader.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_frame_load_type.h"
#include "third_party/blink/public/web/web_history_item.h"
#include "third_party/blink/public/web/web_navigation_params.h"
#include "third_party/blink/public/web/web_optimization_guide_hints.h"
#include "ui/accessibility/ax_tree_id.h"
#include "ui/base/ime/ime_text_span.h"
#include "ui/gfx/range/range.h"
#include "v8/include/v8.h"

namespace gfx {
class Point;
}  // namespace gfx

namespace ui {
struct ImeTextSpan;
}  // namespace ui

namespace blink {

namespace scheduler {
class WebAgentGroupScheduler;
}  // namespace scheduler

class FrameScheduler;
class InterfaceRegistry;
class PageState;
class WebAssociatedURLLoader;
class WebAutofillClient;
class WebContentCaptureClient;
class WebContentSettingsClient;
class WebDocument;
class WebLocalFrameClient;
class WebFrameWidget;
class WebHistoryItem;
class WebInputMethodController;
class WebPerformance;
class WebPlugin;
class WebPrintClient;
class WebRange;
class WebScriptExecutionCallback;
class WebSpellCheckPanelHostClient;
class WebString;
class WebTextCheckClient;
class WebURL;
class WebView;
struct FramePolicy;
struct WebAssociatedURLLoaderOptions;
struct WebConsoleMessage;
struct WebIsolatedWorldInfo;
struct WebPrintPageDescription;
struct WebPrintParams;
struct WebPrintPresetOptions;
struct WebScriptSource;

namespace mojom {
enum class TreeScopeType;
}

// Interface for interacting with in process frames. This contains methods that
// require interacting with a frame's document.
// FIXME: Move lots of methods from WebFrame in here.
class WebLocalFrame : public WebFrame {
 public:
  // Creates a main local frame for the WebView. Can only be invoked when no
  // main frame exists yet. Call Close() to release the returned frame.
  // WebLocalFrameClient may not be null.
  // TODO(dcheng): The argument order should be more consistent with
  // CreateLocalChild() and CreateRemoteChild() in WebRemoteFrame... but it's so
  // painful...
  BLINK_EXPORT static WebLocalFrame* CreateMainFrame(
      WebView*,
      WebLocalFrameClient*,
      blink::InterfaceRegistry*,
      const base::UnguessableToken& frame_token,
      std::unique_ptr<blink::WebPolicyContainer> policy_container,
      WebFrame* opener = nullptr,
      const WebString& name = WebString(),
      network::mojom::WebSandboxFlags = network::mojom::WebSandboxFlags::kNone);

  // Used to create a provisional local frame. Currently, it's possible for a
  // provisional navigation not to commit (i.e. it might turn into a download),
  // but this can only be determined by actually trying to load it. The loading
  // process depends on having a corresponding LocalFrame in Blink to hold all
  // the pending state.
  //
  // When a provisional frame is first created, it is only partially attached to
  // the frame tree. This means that though a provisional frame might have a
  // frame owner, the frame owner's content frame does not point back at the
  // provisional frame. Similarly, though a provisional frame may have a parent
  // frame pointer, the parent frame's children list will not contain the
  // provisional frame. Thus, a provisional frame is invisible to the rest of
  // Blink unless the navigation commits and the provisional frame is fully
  // attached to the frame tree by calling Swap(). It swaps with the
  // |previous_web_frame|.
  //
  // |name| should either match the name of the frame that might be replaced, or
  // be an empty string (e.g. if the browsing context name needs to be cleared
  // due to Cross-Origin Opener Policy).
  //
  // Otherwise, if the load should not commit, call Detach() to discard the
  // frame.
  BLINK_EXPORT static WebLocalFrame* CreateProvisional(
      WebLocalFrameClient*,
      blink::InterfaceRegistry*,
      const base::UnguessableToken& frame_token,
      WebFrame* previous_web_frame,
      const FramePolicy&,
      const WebString& name);

  // Creates a new local child of this frame. Similar to the other methods that
  // create frames, the returned frame should be freed by calling Close() when
  // it's no longer needed.
  virtual WebLocalFrame* CreateLocalChild(
      mojom::TreeScopeType,
      WebLocalFrameClient*,
      blink::InterfaceRegistry*,
      const base::UnguessableToken& frame_token) = 0;

  // Returns the WebFrame associated with the current V8 context. This
  // function can return 0 if the context is associated with a Document that
  // is not currently being displayed in a Frame.
  BLINK_EXPORT static WebLocalFrame* FrameForCurrentContext();

  // Returns the frame corresponding to the given context. This can return 0
  // if the context is detached from the frame, or if the context doesn't
  // correspond to a frame (e.g., workers).
  BLINK_EXPORT static WebLocalFrame* FrameForContext(v8::Local<v8::Context>);

  virtual WebLocalFrameClient* Client() const = 0;

  // Initialization ---------------------------------------------------------

  virtual void SetAutofillClient(WebAutofillClient*) = 0;
  virtual WebAutofillClient* AutofillClient() = 0;

  virtual void SetContentCaptureClient(WebContentCaptureClient*) = 0;
  virtual WebContentCaptureClient* ContentCaptureClient() const = 0;

  // Basic properties ---------------------------------------------------

  LocalFrameToken GetLocalFrameToken() const {
    return LocalFrameToken(GetFrameToken());
  }

  virtual WebDocument GetDocument() const = 0;

  // The name of this frame. If no name is given, empty string is returned.
  virtual WebString AssignedName() const = 0;

  // Sets the name of this frame.
  virtual void SetName(const WebString&) = 0;

  // Returns the AXTreeID associated to the current frame. It is tied to the
  // frame's associated EmbeddingToken, and so it will only be a valid one after
  // the first time that document has been loaded, and will change whenever the
  // loaded document changes (e.g. frame navigated to a different document).
  virtual ui::AXTreeID GetAXTreeID() const = 0;

  // Hierarchy ----------------------------------------------------------

  // Returns true if the current frame is a provisional frame.
  // TODO(https://crbug.com/578349): provisional frames are a hack that should
  // be removed.
  virtual bool IsProvisional() const = 0;

  // Get the highest-level LocalFrame in this frame's in-process subtree.
  virtual WebLocalFrame* LocalRoot() = 0;

  // Returns the WebFrameWidget associated with this frame if there is one or
  // nullptr otherwise.
  // TODO(dcheng): The behavior of this will be changing to always return a
  // WebFrameWidget. Use IsLocalRoot() if it's important to tell if a frame is a
  // local root.
  virtual WebFrameWidget* FrameWidget() const = 0;

  // Creates and returns an associated FrameWidget for this frame. The frame
  // must be a LocalRoot. The WebLocalFrame maintins ownership of the
  // WebFrameWidget that was created.
  BLINK_EXPORT WebFrameWidget* InitializeFrameWidget(
      CrossVariantMojoAssociatedRemote<mojom::FrameWidgetHostInterfaceBase>
          frame_widget_host,
      CrossVariantMojoAssociatedReceiver<mojom::FrameWidgetInterfaceBase>
          frame_widget,
      CrossVariantMojoAssociatedRemote<mojom::WidgetHostInterfaceBase>
          widget_host,
      CrossVariantMojoAssociatedReceiver<mojom::WidgetInterfaceBase> widget,
      const viz::FrameSinkId& frame_sink_id,
      bool is_for_nested_main_frame = false,
      bool hidden = false,
      bool never_composited = false);

  // Returns the frame identified by the given name.  This method supports
  // pseudo-names like _self, _top, and _blank and otherwise performs the same
  // kind of lookup what |window.open(..., name)| would in Javascript.
  virtual WebFrame* FindFrameByName(const WebString& name) = 0;

  // Sets an embedding token for the document in this frame. This token is
  // propagated to the remote parent of this frame (via the browser) such
  // that it can uniquely refer to the document in this frame.
  virtual void SetEmbeddingToken(
      const base::UnguessableToken& embedding_token) = 0;

  // Returns the embedding token for this frame or nullopt if the frame hasn't
  // committed a navigation. This token changes when a new document is committed
  // in this WebLocalFrame.
  virtual const base::Optional<base::UnguessableToken>& GetEmbeddingToken()
      const = 0;

  // Navigation Ping --------------------------------------------------------

  virtual void SendPings(const WebURL& destination_url) = 0;

  // Navigation ----------------------------------------------------------

  // Start reloading the current document.
  // Note: StartReload() will be deprecated, use StartNavigation() instead.
  virtual void StartReload(WebFrameLoadType) = 0;

  // View-source rendering mode.  Set this before loading an URL to cause
  // it to be rendered in view-source mode.
  virtual void EnableViewSourceMode(bool) = 0;
  virtual bool IsViewSourceModeEnabled() const = 0;

  // Returns the document loader that is currently loaded.
  virtual WebDocumentLoader* GetDocumentLoader() const = 0;

  // Sets the referrer for the given request to be the specified URL or
  // if that is null, then it sets the referrer to the referrer that the
  // frame would use for subresources.  NOTE: This method also filters
  // out invalid referrers (e.g., it is invalid to send a HTTPS URL as
  // the referrer for a HTTP request).
  virtual void SetReferrerForRequest(WebURLRequest&, const WebURL&) = 0;

  // The frame should handle the request as a download.
  // If the request is for a blob: URL, a BlobURLToken should be provided
  // as |blob_url_token| to ensure the correct blob gets downloaded.
  virtual void DownloadURL(
      const WebURLRequest& request,
      network::mojom::RedirectMode cross_origin_redirect_behavior,
      CrossVariantMojoRemote<mojom::BlobURLTokenInterfaceBase>
          blob_url_token) = 0;

  // Navigation State -------------------------------------------------------

  // Returns true if there is a pending redirect or location change
  // within specified interval. This could be caused by:
  // * an HTTP Refresh header
  // * an X-Frame-Options header
  // * the respective http-equiv meta tags
  // * window.location value being mutated
  // * CSP policy block
  // * reload
  // * form submission
  virtual bool IsNavigationScheduledWithin(base::TimeDelta interval) const = 0;

  virtual void BlinkFeatureUsageReport(blink::mojom::WebFeature feature) = 0;

  // CSS3 Paged Media ----------------------------------------------------

  // Returns the type of @page size styling for the given page.
  virtual PageSizeType GetPageSizeType(uint32_t page_index) = 0;

  // Gets the description for the specified page. This includes preferred page
  // size and margins in pixels, assuming 96 pixels per inch. The size and
  // margins must be initialized to the default values that are used if auto is
  // specified.
  virtual void GetPageDescription(uint32_t page_index,
                                  WebPrintPageDescription*) = 0;

  // Scripting --------------------------------------------------------------

  // Executes script in the context of the current page.
  virtual void ExecuteScript(const WebScriptSource&) = 0;

  // Executes JavaScript in a new world associated with the web frame.
  // The script gets its own global scope and its own prototypes for
  // intrinsic JavaScript objects (String, Array, and so-on). It also
  // gets its own wrappers for all DOM nodes and DOM constructors.
  //
  // worldID must be > 0 (as 0 represents the main world).
  // worldID must be < kEmbedderWorldIdLimit, high number used internally.
  virtual void ExecuteScriptInIsolatedWorld(int32_t world_id,
                                            const WebScriptSource&) = 0;

  // worldID must be > 0 (as 0 represents the main world).
  // worldID must be < kEmbedderWorldIdLimit, high number used internally.
  // DEPRECATED: Use WebLocalFrame::requestExecuteScriptInIsolatedWorld.
  WARN_UNUSED_RESULT virtual v8::Local<v8::Value>
  ExecuteScriptInIsolatedWorldAndReturnValue(int32_t world_id,
                                             const WebScriptSource&) = 0;

  // Clears the isolated world CSP stored for |world_id| by this frame's
  // Document.
  virtual void ClearIsolatedWorldCSPForTesting(int32_t world_id) = 0;

  // Executes script in the context of the current page and returns the value
  // that the script evaluated to.
  // DEPRECATED: Use WebLocalFrame::requestExecuteScriptAndReturnValue.
  virtual v8::Local<v8::Value> ExecuteScriptAndReturnValue(
      const WebScriptSource&) = 0;

  // Call the function with the given receiver and arguments
  virtual v8::MaybeLocal<v8::Value> ExecuteMethodAndReturnValue(
      v8::Local<v8::Function>,
      v8::Local<v8::Value>,
      int argc,
      v8::Local<v8::Value> argv[]) = 0;

  // Call the function with the given receiver and arguments, bypassing
  // canExecute().
  virtual v8::MaybeLocal<v8::Value> CallFunctionEvenIfScriptDisabled(
      v8::Local<v8::Function>,
      v8::Local<v8::Value>,
      int argc,
      v8::Local<v8::Value> argv[]) = 0;

  // Returns the V8 context for associated with the main world and this
  // frame. There can be many V8 contexts associated with this frame, one for
  // each isolated world and one for the main world. If you don't know what
  // the "main world" or an "isolated world" is, then you probably shouldn't
  // be calling this API.
  virtual v8::Local<v8::Context> MainWorldScriptContext() const = 0;

  // Returns the world ID associated with |script_context|.
  virtual int32_t GetScriptContextWorldId(
      v8::Local<v8::Context> script_context) const = 0;

  // Executes script in the context of the current page and returns the value
  // that the script evaluated to with callback. Script execution can be
  // suspend.
  // DEPRECATED: Prefer requestExecuteScriptInIsolatedWorld().
  virtual void RequestExecuteScriptAndReturnValue(
      const WebScriptSource&,
      bool user_gesture,
      WebScriptExecutionCallback*) = 0;

  // Requests execution of the given function, but allowing for script
  // suspension and asynchronous execution.
  virtual void RequestExecuteV8Function(v8::Local<v8::Context>,
                                        v8::Local<v8::Function>,
                                        v8::Local<v8::Value> receiver,
                                        int argc,
                                        v8::Local<v8::Value> argv[],
                                        WebScriptExecutionCallback*) = 0;

  enum ScriptExecutionType {
    // Execute script synchronously, unless the page is suspended.
    kSynchronous,
    // Execute script asynchronously.
    kAsynchronous,
    // Execute script asynchronously, blocking the window.onload event.
    kAsynchronousBlockingOnload
  };

  // worldID must be > 0 (as 0 represents the main world).
  // worldID must be < kEmbedderWorldIdLimit, high number used internally.
  virtual void RequestExecuteScriptInIsolatedWorld(
      int32_t world_id,
      const WebScriptSource* source_in,
      unsigned num_sources,
      bool user_gesture,
      ScriptExecutionType,
      WebScriptExecutionCallback*) = 0;

  // Logs to the console associated with this frame. If |discard_duplicates| is
  // set, the message will only be added if it is unique (i.e. has not been
  // added to the console previously from this page).
  void AddMessageToConsole(const WebConsoleMessage& message,
                           bool discard_duplicates = false) {
    AddMessageToConsoleImpl(message, discard_duplicates);
  }

  void AddInspectorIssue(mojom::InspectorIssueCode code) {
    AddInspectorIssueImpl(code);
  }

  // Expose modal dialog methods to avoid having to go through JavaScript.
  virtual void Alert(const WebString& message) = 0;
  virtual bool Confirm(const WebString& message) = 0;
  virtual WebString Prompt(const WebString& message,
                           const WebString& default_value) = 0;

  // Debugging -----------------------------------------------------------

  virtual void BindDevToolsAgent(
      CrossVariantMojoAssociatedRemote<mojom::DevToolsAgentHostInterfaceBase>
          devtools_agent_host_remote,
      CrossVariantMojoAssociatedReceiver<mojom::DevToolsAgentInterfaceBase>
          devtools_agent_receiver) = 0;

  // Editing -------------------------------------------------------------
  virtual void UnmarkText() = 0;
  virtual bool HasMarkedText() const = 0;

  virtual WebRange MarkedRange() const = 0;

  // Returns the text range rectangle in the viepwort coordinate space.
  virtual bool FirstRectForCharacterRange(unsigned location,
                                          unsigned length,
                                          gfx::Rect&) const = 0;

  // Supports commands like Undo, Redo, Cut, Copy, Paste, SelectAll,
  // Unselect, etc. See EditorCommand.cpp for the full list of supported
  // commands.
  virtual bool ExecuteCommand(const WebString&) = 0;
  virtual bool ExecuteCommand(const WebString&, const WebString& value) = 0;
  virtual bool IsCommandEnabled(const WebString&) const = 0;

  // Returns the text direction at the start and end bounds of the current
  // selection.  If the selection range is empty, it returns false.
  virtual bool SelectionTextDirection(base::i18n::TextDirection& start,
                                      base::i18n::TextDirection& end) const = 0;
  // Returns true if the selection range is nonempty and its anchor is first
  // (i.e its anchor is its start).
  virtual bool IsSelectionAnchorFirst() const = 0;
  // Changes the text direction of the selected input node.
  virtual void SetTextDirectionForTesting(
      base::i18n::TextDirection direction) = 0;

  // Selection -----------------------------------------------------------

  virtual bool HasSelection() const = 0;

  virtual WebRange SelectionRange() const = 0;

  virtual WebString SelectionAsText() const = 0;
  virtual WebString SelectionAsMarkup() const = 0;

  virtual void TextSelectionChanged(const WebString& selection_text,
                                    uint32_t offset,
                                    const gfx::Range& range) = 0;

  // Expands the selection to a word around the caret and returns
  // true. Does nothing and returns false if there is no caret or
  // there is ranged selection.
  virtual bool SelectWordAroundCaret() = 0;

  // DEPRECATED: Use moveRangeSelection.
  virtual void SelectRange(const gfx::Point& base,
                           const gfx::Point& extent) = 0;

  enum HandleVisibilityBehavior {
    // Hide handle(s) in the new selection.
    kHideSelectionHandle,
    // Show handle(s) in the new selection.
    kShowSelectionHandle,
    // Keep the current handle visibility.
    kPreserveHandleVisibility,
  };

  virtual void SelectRange(const WebRange&,
                           HandleVisibilityBehavior,
                           mojom::SelectionMenuBehavior) = 0;

  virtual WebString RangeAsText(const WebRange&) = 0;

  // Move the current selection to the provided viewport point/points. If the
  // current selection is editable, the new selection will be restricted to
  // the root editable element.
  // |TextGranularity| represents character wrapping granularity. If
  // WordGranularity is set, WebFrame extends selection to wrap word.
  virtual void MoveRangeSelection(
      const gfx::Point& base,
      const gfx::Point& extent,
      WebFrame::TextGranularity = kCharacterGranularity) = 0;
  virtual void MoveCaretSelection(const gfx::Point&) = 0;

  virtual bool SetEditableSelectionOffsets(int start, int end) = 0;
  virtual bool AddImeTextSpansToExistingText(
      const WebVector<ui::ImeTextSpan>& ime_text_spans,
      unsigned text_start,
      unsigned text_end) = 0;
  virtual bool ClearImeTextSpansByType(ui::ImeTextSpan::Type type,
                                       unsigned text_start,
                                       unsigned text_end) = 0;
  virtual bool SetCompositionFromExistingText(
      int composition_start,
      int composition_end,
      const WebVector<ui::ImeTextSpan>& ime_text_spans) = 0;
  virtual void ExtendSelectionAndDelete(int before, int after) = 0;

  // Moves the selection extent point. This function does not allow the
  // selection to collapse. If the new extent is set to the same position as
  // the current base, this function will do nothing.
  virtual void MoveRangeSelectionExtent(const gfx::Point&) = 0;
  // Replaces the selection with the input string.
  virtual void ReplaceSelection(const WebString&) = 0;
  // Deletes text before and after the current cursor position, excluding the
  // selection. The lengths are supplied in UTF-16 Code Unit, not in code points
  // or in glyphs.
  virtual void DeleteSurroundingText(int before, int after) = 0;
  // A variant of deleteSurroundingText(int, int). Major differences are:
  // 1. The lengths are supplied in code points, not in UTF-16 Code Unit or in
  // glyphs.
  // 2. This method does nothing if there are one or more invalid surrogate
  // pairs in the requested range.
  virtual void DeleteSurroundingTextInCodePoints(int before, int after) = 0;

  virtual void ExtractSmartClipData(const gfx::Rect& rect_in_viewport,
                                    WebString& clip_text,
                                    WebString& clip_html,
                                    gfx::Rect& clip_rect) = 0;

  // Spell-checking support -------------------------------------------------
  virtual void SetTextCheckClient(WebTextCheckClient*) = 0;
  virtual void SetSpellCheckPanelHostClient(WebSpellCheckPanelHostClient*) = 0;
  virtual WebSpellCheckPanelHostClient* SpellCheckPanelHostClient() const = 0;
  virtual void ReplaceMisspelledRange(const WebString&) = 0;
  virtual void RemoveSpellingMarkers() = 0;
  virtual void RemoveSpellingMarkersUnderWords(
      const WebVector<WebString>& words) = 0;

  // Content Settings -------------------------------------------------------

  virtual void SetContentSettingsClient(WebContentSettingsClient*) = 0;

  // Image reload -----------------------------------------------------------

  // If the provided node is an image that failed to load, reload it.
  virtual void ReloadImage(const WebNode&) = 0;

  // Iframe sandbox ---------------------------------------------------------

  // Returns false if this frame, or any parent frame is sandboxed and does not
  // have the flag "allow-downloads" set.
  virtual bool IsAllowedToDownload() const = 0;

  // Find-in-page -----------------------------------------------------------

  // Searches a frame for a given string. Only used for testing.
  //
  // If a match is found, this function will select it (scrolling down to
  // make it visible if needed) and fill in selectionRect with the
  // location of where the match was found (in window coordinates).
  //
  // If no match is found, this function clears all tickmarks and
  // highlighting.
  //
  // Returns true if the search string was found, false otherwise.
  virtual bool FindForTesting(int identifier,
                              const WebString& search_text,
                              bool match_case,
                              bool forward,
                              bool new_session,
                              bool force,
                              bool wrap_within_frame,
                              bool async) = 0;

  // Set the tickmarks for the frame and a given `target` element in the frame.
  // If `target` is non-null, use its layout box for scrolling. If `target` is
  // null, use the root layout object for the document in the frame.
  // This will override the default tickmarks generated by find results for the
  // given layout object. If this is called with an empty array, the default
  // behavior will be restored.
  virtual void SetTickmarks(const WebElement& target,
                            const WebVector<gfx::Rect>& tickmarks) = 0;

  // Context menu -----------------------------------------------------------

  // Returns the node that the context menu opened over.
  virtual WebNode ContextMenuNode() const = 0;

  // Copy to the clipboard the image located at a particular point in visual
  // viewport coordinates.
  virtual void CopyImageAtForTesting(const gfx::Point&) = 0;

  // Shows a context menu with the given information from an external context
  // menu request. The given client will be called with the result.
  //
  // The request ID will be returned by this function. This is passed to the
  // client functions for identification.
  //
  // If the client is destroyed, CancelContextMenu() should be called with the
  // request ID returned by this function.
  //
  // Note: if you end up having clients outliving the WebLocalFrame, we should
  // add a CancelContextMenuCallback function that takes a request id.
  virtual void ShowContextMenuFromExternal(
      const UntrustworthyContextMenuParams& params,
      CrossVariantMojoAssociatedRemote<
          blink::mojom::ContextMenuClientInterfaceBase>
          context_menu_client) = 0;

  // Events --------------------------------------------------------------

  // Usage count for chrome.loadtimes deprecation.
  // This will be removed following the deprecation. See: crbug.com/621512
  virtual void UsageCountChromeLoadTimes(const WebString& metric) = 0;

  // Whether we've dispatched "pagehide" on the current document in this frame
  // previously, and haven't dispatched the "pageshow" event after the last time
  // we dispatched "pagehide". This means that we've navigated away from the
  // document and it's still hidden (possibly preserved in the back-forward
  // cache, or unloaded).
  virtual bool DispatchedPagehideAndStillHidden() const = 0;

  // Scheduling ---------------------------------------------------------------

  // Returns FrameScheduler
  virtual FrameScheduler* Scheduler() const = 0;

  // Returns AgentGroupScheduler
  virtual scheduler::WebAgentGroupScheduler* GetAgentGroupScheduler() const = 0;

  // Task queues --------------------------------------------------------------

  // Returns frame-specific task runner to run tasks of this type on.
  // They have the same lifetime as the frame.
  virtual scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(
      TaskType) = 0;

  // Returns the WebInputMethodController associated with this local frame.
  virtual WebInputMethodController* GetInputMethodController() = 0;

  // Loading ------------------------------------------------------------------

  // Returns an AssociatedURLLoader that is associated with this frame.  The
  // loader will, for example, be cancelled when StopLoading is called.
  //
  // FIXME: StopLoading does not yet cancel an associated loader!!
  virtual std::unique_ptr<WebAssociatedURLLoader> CreateAssociatedURLLoader(
      const WebAssociatedURLLoaderOptions&) = 0;

  // This API is deprecated and only required by PepperURLLoaderHost::Close()
  // and PepperPluginInstanceImpl::HandleDocumentLoad() and so it should not be
  // used on a regular basis.
  virtual void DeprecatedStopLoading() = 0;

  // Geometry -----------------------------------------------------------------

  // NOTE: These routines do not force page layout so their results may
  // not be accurate if the page layout is out-of-date.

  // The scroll offset from the top-left corner of the frame in pixels.
  virtual WebSize GetScrollOffset() const = 0;
  virtual void SetScrollOffset(const WebSize&) = 0;

  // The size of the document in this frame.
  virtual WebSize DocumentSize() const = 0;

  // Returns true if the contents (minus scrollbars) has non-zero area.
  virtual bool HasVisibleContent() const = 0;

  // Returns the visible content rect (minus scrollbars), relative to the
  // document.
  virtual gfx::Rect VisibleContentRect() const = 0;

  // Printing ------------------------------------------------------------

  // Dispatch |beforeprint| event, and execute event handlers. They might detach
  // this frame from the owner WebView.
  // This function should be called before pairs of PrintBegin() and PrintEnd().
  // |print_client| is an optional weak pointer to the caller.
  virtual void DispatchBeforePrintEvent(
      base::WeakPtr<WebPrintClient> print_client) = 0;

  // Get the plugin to print, if any. The |constrain_to_node| parameter is the
  // same as the one for PrintBegin() below.
  virtual WebPlugin* GetPluginToPrint(const WebNode& constrain_to_node) = 0;

  // Reformats the WebFrame for printing. WebPrintParams specifies the printable
  // content size, paper size, printable area size, printer DPI and print
  // scaling option. If |constrain_to_node| is specified, then only the given
  // node is printed (for now only plugins are supported), instead of the entire
  // frame.
  // Returns the number of pages that can be printed at the given page size.
  virtual uint32_t PrintBegin(const WebPrintParams& print_params,
                              const WebNode& constrain_to_node) = 0;

  // Returns the page shrinking factor calculated by webkit (usually
  // between 1/1.33 and 1/2). Returns 0 if the page number is invalid or
  // not in printing mode.
  virtual float GetPrintPageShrink(uint32_t page) = 0;

  // Prints one page, and returns the calculated page shrinking factor
  // (usually between 1/1.33 and 1/2).  Returns 0 if the page number is
  // invalid or not in printing mode.
  virtual float PrintPage(uint32_t page_to_print, cc::PaintCanvas*) = 0;

  // Reformats the WebFrame for screen display.
  virtual void PrintEnd() = 0;

  // Dispatch |afterprint| event, and execute event handlers. They might detach
  // this frame from the owner WebView.
  // This function should be called after pairs of PrintBegin() and PrintEnd().
  virtual void DispatchAfterPrintEvent() = 0;

  // Returns true on success and sets the out parameter to the print preset
  // options for the document.
  virtual bool GetPrintPresetOptionsForPlugin(const WebNode&,
                                              WebPrintPresetOptions*) = 0;

  // Paint Preview ------------------------------------------------------------

  // Captures a full frame paint preview of the WebFrame including subframes. If
  // |include_linked_destinations| is true, the capture will include annotations
  // about linked destinations within the document.
  virtual bool CapturePaintPreview(const gfx::Rect& bounds,
                                   cc::PaintCanvas* canvas,
                                   bool include_linked_destinations) = 0;

  // Focus --------------------------------------------------------------

  // Returns whether the keyboard should be suppressed for the currently focused
  // element.
  virtual bool ShouldSuppressKeyboardForFocusedElement() = 0;

  // Performance --------------------------------------------------------

  virtual WebPerformance Performance() const = 0;

  // Ad Tagging ---------------------------------------------------------

  // True if the frame is thought (heuristically) to be created for
  // advertising purposes.
  bool IsAdSubframe() const override = 0;

  // See blink::LocalFrame::SetIsAdSubframe()
  virtual void SetIsAdSubframe(blink::mojom::AdFrameType ad_frame_type) = 0;

  // True iff a script tagged as an ad was on the v8 stack when the frame was
  // created and the frame is a subframe. This is not currently propagated when
  // a frame navigates cross-origin.
  // TODO(crbug.com/1145634): propagate this bit for a frame that navigates
  // cross-origin.
  virtual bool IsSubframeCreatedByAdScript() = 0;

  // User activation -----------------------------------------------------------

  // See blink::LocalFrame::NotifyUserActivation().
  virtual void NotifyUserActivation(
      mojom::UserActivationNotificationType notification_type) = 0;

  // See blink::LocalFrame::HasStickyUserActivation().
  virtual bool HasStickyUserActivation() = 0;

  // See blink::LocalFrame::HasTransientUserActivation().
  virtual bool HasTransientUserActivation() = 0;

  // See blink::LocalFrame::ConsumeTransientUserActivation().
  virtual bool ConsumeTransientUserActivation(
      UserActivationUpdateSource update_source =
          UserActivationUpdateSource::kRenderer) = 0;

  // Optimization Guide --------------------------------------------------------

  // Sets the optimization hints provided by the optimization guide service. See
  // //components/optimization_guide/README.md.
  virtual void SetOptimizationGuideHints(const WebOptimizationGuideHints&) = 0;

  // Testing ------------------------------------------------------------------

  // Get the total spool size (the bounding box of all the pages placed after
  // oneanother vertically), when printing for testing. Even if we still only
  // support a uniform page size, some pages may be rotated using
  // page-orientation.
  virtual gfx::Size SpoolSizeInPixelsForTesting(
      const gfx::Size& page_size_in_pixels,
      uint32_t page_count) = 0;

  // Prints the frame into the canvas, with page boundaries drawn as one pixel
  // wide blue lines. This method exists to support web tests.
  virtual void PrintPagesForTesting(cc::PaintCanvas*,
                                    const gfx::Size& page_size_in_pixels,
                                    const gfx::Size& spool_size_in_pixels) = 0;

  // Returns the bounds rect for current selection. If selection is performed
  // on transformed text, the rect will still bound the selection but will
  // not be transformed itself. If no selection is present, the rect will be
  // empty ((0,0), (0,0)).
  virtual gfx::Rect GetSelectionBoundsRectForTesting() const = 0;

  // Returns the position of the frame's origin relative to the viewport (ie the
  // local root).
  virtual gfx::Point GetPositionInViewportForTesting() const = 0;

  virtual void WasHidden() = 0;
  virtual void WasShown() = 0;

  // Grants ability to lookup a named frame via the FindFrame
  // WebLocalFrameClient API. Enhanced binding security checks that check the
  // agent cluster will be enabled for windows that do not have this permission.
  // This should only be used for extensions and the webview tag.
  virtual void SetAllowsCrossBrowsingInstanceFrameLookup() = 0;

  virtual void SetTargetToCurrentHistoryItem(const WebString& target) = 0;
  virtual void UpdateCurrentHistoryItem() = 0;
  virtual PageState CurrentHistoryItemToPageState() = 0;
  virtual const WebHistoryItem& GetCurrentHistoryItem() const = 0;
  // Reset TextFinder state and loads about:blank.
  virtual void ResetForTesting() = 0;

 protected:
  explicit WebLocalFrame(mojom::TreeScopeType scope,
                         const base::UnguessableToken& frame_token)
      : WebFrame(scope, frame_token) {}

  // Inherited from WebFrame, but intentionally hidden: it never makes sense
  // to directly call these on a WebLocalFrame.
  bool IsWebLocalFrame() const override = 0;
  WebLocalFrame* ToWebLocalFrame() override = 0;
  bool IsWebRemoteFrame() const override = 0;
  WebRemoteFrame* ToWebRemoteFrame() override = 0;

  virtual void AddMessageToConsoleImpl(const WebConsoleMessage&,
                                       bool discard_duplicates) = 0;
  virtual void AddInspectorIssueImpl(blink::mojom::InspectorIssueCode code) = 0;

  virtual void CreateFrameWidgetInternal(
      base::PassKey<WebLocalFrame> pass_key,
      CrossVariantMojoAssociatedRemote<mojom::FrameWidgetHostInterfaceBase>
          frame_widget_host,
      CrossVariantMojoAssociatedReceiver<mojom::FrameWidgetInterfaceBase>
          frame_widget,
      CrossVariantMojoAssociatedRemote<mojom::WidgetHostInterfaceBase>
          widget_host,
      CrossVariantMojoAssociatedReceiver<mojom::WidgetInterfaceBase> widget,
      const viz::FrameSinkId& frame_sink_id,
      bool is_for_nested_main_frame = false,
      bool hidden = false,
      bool never_composited = false) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_
