#include "GitlabClient.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <mbedtls/base64.h>
#include <esp_heap_caps.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG_TRACE


static const size_t MAX_DOWNLOAD_BYTES = 6UL * 1024UL * 1024UL;
static const size_t INITIAL_DOWNLOAD_CAPACITY = 16UL * 1024UL;
static const uint32_t HTTP_TIMEOUT_MS = 30000;
// Buffered JSON/Base64 upload; conservative limit protects ESP32 internal heap.
static const size_t MAX_UPLOAD_BYTES = 256UL * 1024UL;

GitlabClient::GitlabClient(WiFiClientSecure& tlsClient)
  : _tls(tlsClient) {}

void GitlabClient::setError(const String& message) {
  _lastError = message;
  Serial.print("[GitLab] ERROR: ");
  Serial.println(message);
}

String GitlabClient::apiBase() const {
  // Reference Gitlab OTA link to version.txt
  //https://gitlab.com/api/v4/projects/88009717/packages/generic/firmware-index/stable/version.txt";

  String base = _config.gitlabUrl;
  while (base.endsWith("/")) base.remove(base.length() - 1);
  return base + "/api/v4/projects/" + _config.projectId;
}

bool GitlabClient::begin(const GitlabConfig& config, const String& accessToken) {
  _ready = false;
  _lastError = "";

  if (config.projectId.length() == 0) {
    setError("projectId is empty.");
    return false;
  }
  if (accessToken.length() == 0) {
    setError("GitLab token is empty.");
    return false;
  }
  if (config.rootCA.indexOf("-----BEGIN CERTIFICATE-----") < 0 || config.rootCA.indexOf("REPLACE_WITH_VERIFIED_ROOT_CA_PEM") >= 0) {
    setError("A verified root CA PEM must be supplied in GitlabConfig.rootCA.");
    return false;
  }
  if (!config.gitlabUrl.startsWith("https://")) {
    setError("Only HTTPS GitLab URLs are accepted.");
    return false;
  }

  _config = config;
  _token = accessToken;
  _tls.setCACert(_config.rootCA.c_str());
  _tls.setTimeout(HTTP_TIMEOUT_MS);
  _ready = true;
  return true;
}

String GitlabClient::encodeQuery(const String& input) const {
  const char* hex = "0123456789ABCDEF";
  String out;
  out.reserve(input.length() * 3);
  for (size_t i = 0; i < input.length(); ++i) {
    uint8_t c = (uint8_t)input[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else {
      out += '%';
      /***** 
*  c >> 4 = shift right 4 bits / divide by 16. It moves the high nibble down to low nibble.
*  c = 0xAB
*  hex[(c >> 4) & 0x0F] // 0xA -> high nibble
*  hex[c & 0x0F]        // 0xB -> low nibble
*****/
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

// GitLab repository file endpoint requires the entire repository path encoded
// as one path parameter: assets/logo.png -> assets%2Flogo.png.
String GitlabClient::encodePathForFileEndpoint(const String& input) const {
  return encodeQuery(input);
}

bool GitlabClient::requestJsonPage(const String& url, String& response, int& statusCode) {
  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(_tls, url)) {
    setError("HTTPClient begin() failed.");
    return false;
  }

  http.addHeader("PRIVATE-TOKEN", _token);
  http.addHeader("Accept", "application/json");
  statusCode = http.GET();

  if (statusCode == 401) setError("HTTP 401: token invalid, expired, or not accepted.");
  else if (statusCode == 403) setError("HTTP 403: token lacks permission for this project.");
  else if (statusCode == 404) setError("HTTP 404: project or repository path not found, or access is concealed.");
  else if (statusCode < 200 || statusCode >= 300) {
    setError("GitLab API returned HTTP " + String(statusCode));
  }

  if (statusCode == HTTP_CODE_OK) {
    response = http.getString();  // JSON metadata only, not file payload.
  }
  http.end();
  return statusCode == HTTP_CODE_OK;
}

bool GitlabClient::listFiles(const String& path,
                             std::vector<String>& outFileNames,
                             bool recursive) {
  outFileNames.clear();
  if (!_ready) {
    setError("Call begin() successfully before listFiles().");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    setError("Wi-Fi is not connected.");
    return false;
  }

  // Use offset pagination; GitLab's documented maximum per_page is 100.
  for (uint32_t page = 1; page <= 10000; ++page) {
    String url = apiBase() + "/repository/tree?ref=" + encodeQuery(_config.branch) + "&per_page=100&page=" + String(page) + "&recursive=" + String(recursive ? "true" : "false");
    if (path.length() > 0 && path != "/") {
      url += "&path=" + encodeQuery(path);
    }

    String body;
    int status = 0;
    if (!requestJsonPage(url, body, status)) return false;

    DynamicJsonDocument doc(24 * 1024);
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      setError(String("Failed to parse repository tree JSON: ") + err.c_str());
      return false;
    }
    if (!doc.is<JsonArray>()) {
      setError("Unexpected repository tree response format.");
      return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject entry : arr) {
      const char* type = entry["type"] | "";
      const char* name = entry["name"] | "";
      /*
      if (strcmp(type, "blob") == 0 && name[0] != '\0') {
        outFileNames.push_back(String(name));
      }
*/

      if (name[0] != '\0') {
        if (strcmp(type, "blob") == 0) {
          // Regular file
          outFileNames.push_back(String(name));
        } else if (strcmp(type, "tree") == 0) {
          // Directory
          outFileNames.push_back(String(name) + "/");
        }
      }
    }


    // A short page indicates the last page. Empty page also stops.
    if (arr.size() < 100) return true;
  }

  setError("Pagination safety limit reached (10000 pages).");
  return false;
}

bool GitlabClient::downloadFileToPSRAM(const String& repoFilePath,
                                       uint8_t** outBuffer,
                                       size_t* outLen) {
  if (outBuffer == nullptr || outLen == nullptr) {
    setError("Output pointers must not be null.");
    return false;
  }
  *outBuffer = nullptr;
  *outLen = 0;

  if (!_ready) {
    setError("Call begin() successfully before downloadFileToPSRAM().");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    setError("Wi-Fi is not connected.");
    return false;
  }
  if (repoFilePath.length() == 0 || repoFilePath.startsWith("/")) {
    setError("Provide a repository-relative file path without a leading slash.");
    return false;
  }

  String url = apiBase() + "/repository/files/" + encodePathForFileEndpoint(repoFilePath) + "/raw?ref=" + encodeQuery(_config.branch);

  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setReuse(false);

  if (!http.begin(_tls, url)) {
    setError("HTTPClient begin() failed for raw file request.");
    return false;
  }
  http.addHeader("PRIVATE-TOKEN", _token);
  http.addHeader("Accept", "application/octet-stream");

#ifdef DEBUG_TRACE
  Serial.printf("[GitLab] Download path: '%s'\n", repoFilePath.c_str());
  Serial.printf("[GitLab] Request URL: %s\n", url.c_str());

  int status = http.GET();

  Serial.printf("[GitLab] HTTP status: %d\n", status);
#else

  int status = http.GET();
#endif

  if (status != HTTP_CODE_OK) {
    if (status == 401) setError("HTTP 401: token invalid or expired.");
    else if (status == 403) setError("HTTP 403: insufficient repository permission.");
    else if (status == 404) setError("HTTP 404: file not found, branch missing, or access concealed.");
    else setError("Raw file request returned HTTP " + String(status));
    http.end();
    return false;
  }

  int contentLength = http.getSize();  // -1 if chunked/unknown
  if (contentLength > 0 && (size_t)contentLength > MAX_DOWNLOAD_BYTES) {
    setError("File exceeds configured maximum download size.");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  if (stream == nullptr) {
    setError("HTTP stream unavailable.");
    http.end();
    return false;
  }

  size_t capacity = contentLength > 0 ? (size_t)contentLength : INITIAL_DOWNLOAD_CAPACITY;
  if (capacity == 0) capacity = INITIAL_DOWNLOAD_CAPACITY;
  uint8_t* buffer = (uint8_t*)ps_malloc(capacity);
  if (buffer == nullptr) {
    setError("PSRAM allocation failed for download buffer.");
    http.end();
    return false;
  }

  size_t used = 0;
  uint8_t chunk[2048];
  uint32_t lastProgress = millis();
  uint32_t lastDataAt = millis();

  while (http.connected() || stream->available()) {
    int available = stream->available();
    if (available > 0) {
      size_t want = (size_t)available;
      if (want > sizeof(chunk)) want = sizeof(chunk);
      int got = stream->readBytes(chunk, want);
      if (got <= 0) continue;

      if (used + (size_t)got > MAX_DOWNLOAD_BYTES) {
        free(buffer);
        setError("Download exceeded configured maximum size.");
        http.end();
        return false;
      }

      if (used + (size_t)got > capacity) {
        size_t newCapacity = capacity;
        while (newCapacity < used + (size_t)got) {
          size_t next = newCapacity * 2;
          if (next > MAX_DOWNLOAD_BYTES) next = MAX_DOWNLOAD_BYTES;
          if (next <= newCapacity) {
            free(buffer);
            setError("Download buffer capacity overflow.");
            http.end();
            return false;
          }
          newCapacity = next;
        }

        uint8_t* grown = (uint8_t*)ps_malloc(newCapacity);
        if (grown == nullptr) {
          free(buffer);
          setError("PSRAM reallocation failed while growing download buffer.");
          http.end();
          return false;
        }
        memcpy(grown, buffer, used);
        free(buffer);
        buffer = grown;
        capacity = newCapacity;
      }

      memcpy(buffer + used, chunk, (size_t)got);
      used += (size_t)got;
      lastDataAt = millis();
    } else {
      if (!http.connected()) break;
      if (millis() - lastDataAt > HTTP_TIMEOUT_MS) {
        free(buffer);
        setError("Timed out waiting for download data.");
        http.end();
        return false;
      }
      delay(2);
    }

    if (millis() - lastProgress >= 1000) {
      if (contentLength > 0) {
        Serial.printf("[GitLab] Download: %u / %u bytes (%u%%)\n",
                      (unsigned)used, (unsigned)contentLength,
                      (unsigned)((used * 100ULL) / (size_t)contentLength));
      } else {
        Serial.printf("[GitLab] Download: %u bytes\n", (unsigned)used);
      }
      lastProgress = millis();
    }
  }

  http.end();

  if (contentLength >= 0 && used != (size_t)contentLength) {
    free(buffer);
    setError("Download length mismatch: expected " + String(contentLength) + ", received " + String(used) + ".");
    return false;
  }

  if (used == 0) {
    // HTTP 200 with zero bytes is a successful download of an empty file.
    // Return the allocated buffer so the caller can free() it normally.
    *outBuffer = buffer;
    *outLen = 0;

    Serial.println("[GitLab] Download complete: empty file (0 bytes).");
    return true;
  }


  *outBuffer = buffer;
  *outLen = used;
  Serial.printf("[GitLab] Download complete: %u bytes in PSRAM.\n", (unsigned)used);
  return true;
}


// Escape text before inserting it into a JSON string value.
static String jsonEscape(const String& input) {
  String out;
  out.reserve(input.length() + 16);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) {
          const char hex[] = "0123456789ABCDEF";
          out += "\\u00";
          out += hex[((uint8_t)c >> 4) & 0x0F];
          out += hex[(uint8_t)c & 0x0F];
        } else out += c;
    }
  }
  return out;
}

bool GitlabClient::uploadNewFile(const String& path, const uint8_t* data,
                                 size_t len, const String& message) {
  return writeFileRequest("POST", path, data, len, message);
}

bool GitlabClient::updateFile(const String& path, const uint8_t* data,
                              size_t len, const String& message) {
  return writeFileRequest("PUT", path, data, len, message);
}

bool GitlabClient::writeFileRequest(const char* method, const String& path,
                                    const uint8_t* data, size_t len,
                                    const String& commitMessage) {
  if (!_ready) {
    setError("Call begin() successfully before file writes.");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    setError("Wi-Fi is not connected.");
    return false;
  }
  if (path.length() == 0 || path.startsWith("/") || path.indexOf("..") >= 0) {
    setError("Use a repository-relative path without a leading slash or '..'.");
    return false;
  }
  if (!data || len == 0) {
    setError("Upload data is null or empty.");
    return false;
  }
  if (len > MAX_UPLOAD_BYTES) {
    setError("Upload exceeds 256 KiB safety limit.");
    return false;
  }
  if (commitMessage.length() == 0) {
    setError("Commit message must not be empty.");
    return false;
  }

  size_t encodedLen = 0;
  int rc = mbedtls_base64_encode(nullptr, 0, &encodedLen, data, len);
  if (rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL && rc != 0) {
    setError("Failed to calculate Base64 output length.");
    return false;
  }
  char* encoded = (char*)ps_malloc(encodedLen + 1);
  if (!encoded) {
    setError("PSRAM allocation failed for Base64 data.");
    return false;
  }
  rc = mbedtls_base64_encode((unsigned char*)encoded, encodedLen + 1, &encodedLen, data, len);
  if (rc != 0) {
    free(encoded);
    setError("Base64 encoding failed.");
    return false;
  }
  encoded[encodedLen] = '\0';


  String payload;
  const size_t payloadCapacity =
    encodedLen + commitMessage.length() + _config.branch.length() + 128;

  if (!payload.reserve(payloadCapacity)) {
    free(encoded);
    setError("Memory allocation failed for JSON upload payload.");
    return false;
  }

  payload += "{\"branch\":\"";
  payload += jsonEscape(_config.branch);
  payload += "\",\"commit_message\":\"";
  payload += jsonEscape(commitMessage);
  payload += "\",\"content\":\"";
  payload += encoded;
  payload += "\",\"encoding\":\"base64\"}";

  free(encoded);
  encoded = nullptr;

  if (payload.length() == 0) {
    setError("Failed to construct JSON upload payload.");
    return false;
  }


  String url = apiBase() + "/repository/files/" + encodePathForFileEndpoint(path);

#ifdef DEBUG_TRACE
  Serial.println();
  Serial.println("[GitLab] ===== FILE WRITE DEBUG =====");
  Serial.printf("[GitLab] Method: %s\n", method);
  Serial.printf("[GitLab] Branch: '%s'\n", _config.branch.c_str());
  Serial.printf("[GitLab] Repository path: '%s'\n", path.c_str());
  Serial.printf("[GitLab] Encoded path: '%s'\n",
                encodePathForFileEndpoint(path).c_str());
  Serial.printf("[GitLab] Request URL: %s\n", url.c_str());
  Serial.printf("[GitLab] Payload length: %u bytes\n",
                (unsigned)payload.length());
  Serial.println("[GitLab] ============================");
#endif

  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setReuse(false);
  if (!http.begin(_tls, url)) {
    setError("HTTPClient begin() failed for file write.");
    return false;
  }
  http.addHeader("PRIVATE-TOKEN", _token);
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  int status = http.sendRequest(method, (uint8_t*)payload.c_str(), payload.length());
  String response = http.getString();

#ifdef DEBUG_TRACE
  Serial.printf("[GitLab] HTTP status: %d\n", status);
  Serial.printf("[GitLab] Response: %s\n", response.c_str());
#endif

  http.end();
  payload = "";

  if (status >= 200 && status < 300) {
    Serial.printf("[GitLab] %s succeeded for '%s' (HTTP %d).\n",
                  strcmp(method, "POST") == 0 ? "Upload" : "Update", path.c_str(), status);
    return true;
  }
  if (status == 400) setError("HTTP 400: request invalid; POST may target an existing file or PUT a missing file. " + response);
  else if (status == 401) setError("HTTP 401: token invalid, expired, or not accepted.");
  else if (status == 403) setError("HTTP 403: token or project role lacks permission.");
  else if (status == 404) setError("HTTP 404: project, branch, or endpoint not found, or access concealed.");
  else if (status == 409) setError("HTTP 409: repository conflict. " + response);
  else setError("GitLab file write returned HTTP " + String(status) + ". " + response);
  return false;
}

bool GitlabClient::deleteRemoteFile(const String& path, const String& message) {
  return deleteFileRequest(path, message);
}

bool GitlabClient::deleteFileRequest(const String& path, const String& commitMessage) {
  if (!_ready) {
    setError("Call begin() successfully before deleting remote files.");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    setError("Wi-Fi is not connected.");
    return false;
  }
  if (path.length() == 0 || path.startsWith("/") || path.indexOf("..") >= 0) {
    setError("Use a repository-relative path without a leading slash or '..'.");
    return false;
  }
  if (commitMessage.length() == 0) {
    setError("Commit message must not be empty.");
    return false;
  }

  String payload;
  payload.reserve(commitMessage.length() + _config.branch.length() + 64);
  payload += "{\"branch\":\"";
  payload += jsonEscape(_config.branch);
  payload += "\",\"commit_message\":\"";
  payload += jsonEscape(commitMessage);
  payload += "\"}";

  String url = apiBase() + "/repository/files/" + encodePathForFileEndpoint(path);
  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setReuse(false);
  if (!http.begin(_tls, url)) {
    setError("HTTPClient begin() failed for file deletion.");
    return false;
  }
  http.addHeader("PRIVATE-TOKEN", _token);
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  int status = http.sendRequest("DELETE", (uint8_t*)payload.c_str(), payload.length());
  String response = http.getString();
  http.end();

  if (status >= 200 && status < 300) {
    Serial.printf("[GitLab] Remote delete succeeded for '%s' (HTTP %d).\n", path.c_str(), status);
    return true;
  }
  if (status == 401) setError("HTTP 401: token invalid, expired, or not accepted.");
  else if (status == 403) setError("HTTP 403: token or project role lacks delete permission.");
  else if (status == 404) setError("HTTP 404: project, branch, or file not found, or access concealed.");
  else setError("GitLab file delete returned HTTP " + String(status) + ". " + response);
  return false;
}
