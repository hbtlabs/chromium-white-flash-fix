// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_
#define CHROME_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_

#include <stddef.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "content/public/browser/content_browser_client.h"
#include "extensions/features/features.h"
#include "media/media_features.h"

class ChromeContentBrowserClientParts;

namespace base {
class CommandLine;
}

namespace content {
class BrowserContext;
class QuotaPermissionContext;
}

namespace extensions {
class BrowserPermissionsPolicyDelegate;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace version_info {
enum class Channel;
}

namespace url {
class Origin;
}

class ChromeContentBrowserClient : public content::ContentBrowserClient {
 public:
  ChromeContentBrowserClient();
  ~ChromeContentBrowserClient() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Notification that the application locale has changed. This allows us to
  // update our I/O thread cache of this value.
  static void SetApplicationLocale(const std::string& locale);

  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;
  void PostAfterStartupTask(const tracked_objects::Location& from_here,
                            const scoped_refptr<base::TaskRunner>& task_runner,
                            const base::Closure& task) override;
  bool IsBrowserStartupComplete() override;
  std::string GetStoragePartitionIdForSite(
      content::BrowserContext* browser_context,
      const GURL& site) override;
  bool IsValidStoragePartitionId(content::BrowserContext* browser_context,
                                 const std::string& partition_id) override;
  void GetStoragePartitionConfigForSite(
      content::BrowserContext* browser_context,
      const GURL& site,
      bool can_be_default,
      std::string* partition_domain,
      std::string* partition_name,
      bool* in_memory) override;
  content::WebContentsViewDelegate* GetWebContentsViewDelegate(
      content::WebContents* web_contents) override;
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  GURL GetEffectiveURL(content::BrowserContext* browser_context,
                       const GURL& url) override;
  bool ShouldUseProcessPerSite(content::BrowserContext* browser_context,
                               const GURL& effective_url) override;
  bool DoesSiteRequireDedicatedProcess(content::BrowserContext* browser_context,
                                       const GURL& effective_site_url) override;
  bool ShouldLockToOrigin(content::BrowserContext* browser_context,
                          const GURL& effective_site_url) override;
  void GetAdditionalWebUISchemes(
      std::vector<std::string>* additional_schemes) override;
  bool LogWebUIUrl(const GURL& web_ui_url) const override;
  bool IsHandledURL(const GURL& url) override;
  bool CanCommitURL(content::RenderProcessHost* process_host,
                    const GURL& url) override;
  bool ShouldAllowOpenURL(content::SiteInstance* site_instance,
                          const GURL& url) override;
  void OverrideNavigationParams(content::SiteInstance* site_instance,
                                ui::PageTransition* transition,
                                bool* is_renderer_initiated,
                                content::Referrer* referrer) override;
  bool ShouldFrameShareParentSiteInstanceDespiteTopDocumentIsolation(
      const GURL& url,
      content::SiteInstance* parent_site_instance) override;
  bool IsSuitableHost(content::RenderProcessHost* process_host,
                      const GURL& site_url) override;
  bool MayReuseHost(content::RenderProcessHost* process_host) override;
  bool ShouldTryToUseExistingProcessHost(
      content::BrowserContext* browser_context,
      const GURL& url) override;
  void SiteInstanceGotProcess(content::SiteInstance* site_instance) override;
  void SiteInstanceDeleting(content::SiteInstance* site_instance) override;
  bool ShouldSwapBrowsingInstancesForNavigation(
      content::SiteInstance* site_instance,
      const GURL& current_url,
      const GURL& new_url) override;
  bool ShouldSwapProcessesForRedirect(
      content::BrowserContext* browser_context,
      const GURL& current_url,
      const GURL& new_url) override;
  bool ShouldAssignSiteForURL(const GURL& url) override;
  std::string GetCanonicalEncodingNameByAliasName(
      const std::string& alias_name) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  std::string GetApplicationLocale() override;
  std::string GetAcceptLangs(content::BrowserContext* context) override;
  const gfx::ImageSkia* GetDefaultFavicon() override;
  bool IsDataSaverEnabled(content::BrowserContext* context) override;
  bool AllowAppCache(const GURL& manifest_url,
                     const GURL& first_party,
                     content::ResourceContext* context) override;
  bool AllowServiceWorker(const GURL& scope,
                          const GURL& first_party,
                          content::ResourceContext* context,
                          int render_process_id,
                          int render_frame_id) override;
  bool AllowGetCookie(const GURL& url,
                      const GURL& first_party,
                      const net::CookieList& cookie_list,
                      content::ResourceContext* context,
                      int render_process_id,
                      int render_frame_id) override;
  bool AllowSetCookie(const GURL& url,
                      const GURL& first_party,
                      const std::string& cookie_line,
                      content::ResourceContext* context,
                      int render_process_id,
                      int render_frame_id,
                      const net::CookieOptions& options) override;
  bool AllowSaveLocalState(content::ResourceContext* context) override;
  void AllowWorkerFileSystem(
      const GURL& url,
      content::ResourceContext* context,
      const std::vector<std::pair<int, int>>& render_frames,
      base::Callback<void(bool)> callback) override;
  bool AllowWorkerIndexedDB(
      const GURL& url,
      const base::string16& name,
      content::ResourceContext* context,
      const std::vector<std::pair<int, int>>& render_frames) override;
#if defined(ENABLE_WEBRTC)
  bool AllowWebRTCIdentityCache(const GURL& url,
                                const GURL& first_party_url,
                                content::ResourceContext* context) override;
#endif  // defined(ENABLE_WEBRTC)
  bool AllowKeygen(const GURL& url, content::ResourceContext* context) override;
  AllowWebBluetoothResult AllowWebBluetooth(
      content::BrowserContext* browser_context,
      const url::Origin& requesting_origin,
      const url::Origin& embedding_origin) override;
  std::string GetWebBluetoothBlacklist() override;
  net::URLRequestContext* OverrideRequestContextForURL(
      const GURL& url,
      content::ResourceContext* context) override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  std::unique_ptr<storage::QuotaEvictionPolicy>
  GetTemporaryStorageEvictionPolicy(content::BrowserContext* context) override;
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool overridable,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback) override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
  content::MediaObserver* GetMediaObserver() override;
  content::PlatformNotificationService* GetPlatformNotificationService()
      override;
  bool CanCreateWindow(const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const GURL& source_origin,
                       WindowContainerType container_type,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       const std::string& frame_name,
                       WindowOpenDisposition disposition,
                       const blink::WebWindowFeatures& features,
                       bool user_gesture,
                       bool opener_suppressed,
                       content::ResourceContext* context,
                       int render_process_id,
                       int opener_render_view_id,
                       int opener_render_frame_id,
                       bool* no_javascript_access) override;
  void ResourceDispatcherHostCreated() override;
  content::SpeechRecognitionManagerDelegate*
  CreateSpeechRecognitionManagerDelegate() override;
  net::NetLog* GetNetLog() override;
  void OverrideWebkitPrefs(content::RenderViewHost* rvh,
                           content::WebPreferences* prefs) override;
  void BrowserURLHandlerCreated(content::BrowserURLHandler* handler) override;
  void ClearCache(content::RenderFrameHost* rfh) override;
  void ClearCookies(content::RenderFrameHost* rfh) override;
  void ClearSiteData(content::BrowserContext* browser_context,
                     const url::Origin& origin,
                     bool remove_cookies,
                     bool remove_storage,
                     bool remove_cache,
                     const base::Closure& callback) override;
  base::FilePath GetDefaultDownloadDirectory() override;
  std::string GetDefaultDownloadName() override;
  base::FilePath GetShaderDiskCacheDirectory() override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  content::BrowserPpapiHost* GetExternalBrowserPpapiHost(
      int plugin_process_id) override;
  gpu::GpuChannelEstablishFactory* GetGpuChannelEstablishFactory() override;
  bool AllowPepperSocketAPI(
      content::BrowserContext* browser_context,
      const GURL& url,
      bool private_api,
      const content::SocketPermissionRequest* params) override;
  bool IsPepperVpnProviderAPIAllowed(content::BrowserContext* browser_context,
                                     const GURL& url) override;
  std::unique_ptr<content::VpnServiceProxy> GetVpnServiceProxy(
      content::BrowserContext* browser_context) override;
  ui::SelectFilePolicy* CreateSelectFilePolicy(
      content::WebContents* web_contents) override;
  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) override;
  void GetSchemesBypassingSecureContextCheckWhitelist(
      std::set<std::string>* schemes) override;
  void GetURLRequestAutoMountHandlers(
      std::vector<storage::URLRequestAutoMountHandler>* handlers) override;
  void GetAdditionalFileSystemBackends(
      content::BrowserContext* browser_context,
      const base::FilePath& storage_partition_path,
      ScopedVector<storage::FileSystemBackend>* additional_backends) override;
  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;
  content::TracingDelegate* GetTracingDelegate() override;
  bool IsPluginAllowedToCallRequestOSFileHandle(
      content::BrowserContext* browser_context,
      const GURL& url) override;
  bool IsPluginAllowedToUseDevChannelAPIs(
      content::BrowserContext* browser_context,
      const GURL& url) override;
  void OverridePageVisibilityState(
      content::RenderFrameHost* render_frame_host,
      blink::WebPageVisibilityState* visibility_state) override;

#if defined(OS_ANDROID)
  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::FileDescriptorInfo* mappings,
      std::map<int, base::MemoryMappedFile::Region>* regions) override;
#elif defined(OS_POSIX) && !defined(OS_MACOSX)
  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::FileDescriptorInfo* mappings) override;
#endif  // defined(OS_ANDROID)
#if defined(OS_WIN)
  bool PreSpawnRenderer(sandbox::TargetPolicy* policy) override;
  base::string16 GetAppContainerSidForSandboxType(
      int sandbox_type) const override;
#endif
  void ExposeInterfacesToRenderer(
      service_manager::InterfaceRegistry* registry,
      content::RenderProcessHost* render_process_host) override;
  void ExposeInterfacesToMediaService(
      service_manager::InterfaceRegistry* registry,
      content::RenderFrameHost* render_frame_host) override;
  void RegisterRenderFrameMojoInterfaces(
      service_manager::InterfaceRegistry* registry,
      content::RenderFrameHost* render_frame_host) override;
  void ExposeInterfacesToGpuProcess(
      service_manager::InterfaceRegistry* registry,
      content::GpuProcessHost* render_process_host) override;
  void RegisterInProcessServices(StaticServiceMap* services) override;
  void RegisterOutOfProcessServices(
      OutOfProcessServiceMap* services) override;
  std::unique_ptr<base::Value> GetServiceManifestOverlay(
      const std::string& name) override;
  void OpenURL(content::BrowserContext* browser_context,
               const content::OpenURLParams& params,
               const base::Callback<void(content::WebContents*)>& callback)
      override;
  content::PresentationServiceDelegate* GetPresentationServiceDelegate(
      content::WebContents* web_contents) override;
  void RecordURLMetric(const std::string& metric, const GURL& url) override;
  ScopedVector<content::NavigationThrottle> CreateThrottlesForNavigation(
      content::NavigationHandle* handle) override;
  std::unique_ptr<content::NavigationUIData> GetNavigationUIData(
      content::NavigationHandle* navigation_handle) override;
  std::unique_ptr<content::MemoryCoordinatorDelegate>
  GetMemoryCoordinatorDelegate() override;

#if BUILDFLAG(ENABLE_MEDIA_REMOTING)
  void CreateMediaRemoter(content::RenderFrameHost* render_frame_host,
                          media::mojom::RemotingSourcePtr source,
                          media::mojom::RemoterRequest request) final;
#endif  // BUILDFLAG(ENABLE_MEDIA_REMOTING)

 private:
  friend class DisableWebRtcEncryptionFlagTest;

#if defined(ENABLE_WEBRTC)
  // Copies disable WebRTC encryption switch depending on the channel.
  static void MaybeCopyDisableWebRtcEncryptionSwitch(
      base::CommandLine* to_command_line,
      const base::CommandLine& from_command_line,
      version_info::Channel channel);
#endif

  void FileSystemAccessed(
      const GURL& url,
      const std::vector<std::pair<int, int> >& render_frames,
      base::Callback<void(bool)> callback,
      bool allow);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  void GuestPermissionRequestHelper(
      const GURL& url,
      const std::vector<std::pair<int, int> >& render_frames,
      base::Callback<void(bool)> callback,
      bool allow);

  static void RequestFileSystemPermissionOnUIThread(
      int render_process_id,
      int render_frame_id,
      const GURL& url,
      bool allowed_by_default,
      const base::Callback<void(bool)>& callback);
#endif

#if defined(ENABLE_PLUGINS)
  // Set of origins that can use TCP/UDP private APIs from NaCl.
  std::set<std::string> allowed_socket_origins_;
  // Set of origins that can get a handle for FileIO from NaCl.
  std::set<std::string> allowed_file_handle_origins_;
  // Set of origins that can use "dev chanel" APIs from NaCl, even on stable
  // versions of Chrome.
  std::set<std::string> allowed_dev_channel_origins_;
#endif

  // Vector of additional ChromeContentBrowserClientParts.
  // Parts are deleted in the reverse order they are added.
  std::vector<ChromeContentBrowserClientParts*> extra_parts_;

  base::WeakPtrFactory<ChromeContentBrowserClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeContentBrowserClient);
};

#endif  // CHROME_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_
