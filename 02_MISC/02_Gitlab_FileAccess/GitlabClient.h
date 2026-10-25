#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <vector>

struct GitlabConfig {
  String gitlabUrl = "https://gitlab.com";
  String projectId;       // Numeric project ID or URL-encoded group%2Fproject
  String branch = "main";
  String rootCA;          // PEM root CA used by WiFiClientSecure::setCACert()
};

class GitlabClient {
public:
  explicit GitlabClient(WiFiClientSecure& tlsClient);

  bool begin(const GitlabConfig& config, const String& accessToken);

  bool listFiles(const String& path,
                 std::vector<String>& outFileNames,
                 bool recursive = false);

  bool downloadFileToPSRAM(const String& repoFilePath,
                           uint8_t** outBuffer,
                           size_t* outLen);

  String lastError() const { return _lastError; }

private:
  WiFiClientSecure& _tls;
  GitlabConfig _config;
  String _token;
  String _lastError;
  bool _ready = false;

  String apiBase() const;
  String encodeQuery(const String& input) const;
  String encodePathForFileEndpoint(const String& input) const;
  bool requestJsonPage(const String& url, String& response, int& statusCode);
  void setError(const String& message);
};
