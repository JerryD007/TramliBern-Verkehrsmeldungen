// Private mbedTLS size definitions isolated from Arduino C++ headers.
// This firmware is pinned to Espressif core 3.3.0 / IDF 5.5.
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include "ssl_misc.h"
#include "TlsBufferSizes.h"
_Static_assert(MBEDTLS_SSL_IN_BUFFER_LEN==16717 && MBEDTLS_SSL_OUT_BUFFER_LEN==16717,
               "Revalidate admission sizing for changed core configuration");
const size_t RUNDE_TLS_INPUT_BYTES=MBEDTLS_SSL_IN_BUFFER_LEN;
const size_t RUNDE_TLS_OUTPUT_BYTES=MBEDTLS_SSL_OUT_BUFFER_LEN;
