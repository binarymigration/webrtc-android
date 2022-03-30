// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_URL_REQUEST_UTIL_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_URL_REQUEST_UTIL_H_

#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-forward.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-forward.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_mixed_content_context_type.h"

namespace blink {

class WebHTTPBody;
class WebURLRequest;
class WebString;

BLINK_PLATFORM_EXPORT WebString
GetWebURLRequestHeadersAsString(const WebURLRequest& request);

// Takes a ResourceRequestBody and converts into WebHTTPBody.
BLINK_PLATFORM_EXPORT WebHTTPBody
GetWebHTTPBodyForRequestBody(const network::ResourceRequestBody& input);

// Takes a WebHTTPBody and converts into a ResourceRequestBody.
BLINK_PLATFORM_EXPORT scoped_refptr<network::ResourceRequestBody>
GetRequestBodyForWebHTTPBody(const WebHTTPBody& httpBody);

// Takes a WebURLRequest and sets the appropriate information
// in a ResourceRequestBody structure. Returns an empty scoped_refptr
// if the request body is not present.
BLINK_PLATFORM_EXPORT scoped_refptr<network::ResourceRequestBody>
GetRequestBodyForWebURLRequest(const WebURLRequest& request);

// Helper functions to convert enums from the blink type to the content
// type.
BLINK_PLATFORM_EXPORT mojom::RequestContextType
GetRequestContextTypeForWebURLRequest(const WebURLRequest& request);
BLINK_PLATFORM_EXPORT network::mojom::RequestDestination
GetRequestDestinationForWebURLRequest(const WebURLRequest& request);
BLINK_PLATFORM_EXPORT WebMixedContentContextType
GetMixedContentContextTypeForWebURLRequest(const WebURLRequest& request);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_URL_REQUEST_UTIL_H_
