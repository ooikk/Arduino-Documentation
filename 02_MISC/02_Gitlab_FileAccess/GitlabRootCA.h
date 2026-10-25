#pragma once

// SECURITY: Replace this placeholder with a verified PEM root CA certificate
// that validates the current TLS certificate chain presented by gitlab.com.
// Do NOT use setInsecure(), do NOT use a leaf/server certificate as a root.
// Keep the PEM delimiters and line breaks intact.
static const char GITLAB_ROOT_CA[] PROGMEM = R"PEM(
REPLACE_WITH_VERIFIED_ROOT_CA_PEM
)PEM";
