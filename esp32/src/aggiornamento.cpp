#include "aggiornamento.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include "certificati.h"
#include "ota.h"
#include "i18n.h"

static String versioneNuova, urlFirmware, shaAtteso;

static int confronta(const String& a, const String& b) {      // "1.2.10" > "1.2.9"
  int ia = 0, ib = 0;
  while (ia < (int)a.length() || ib < (int)b.length()) {
    long na = a.substring(ia).toInt(), nb = b.substring(ib).toInt();
    if (na != nb) return na < nb ? -1 : 1;
    int pa = a.indexOf('.', ia), pb = b.indexOf('.', ib);
    ia = pa < 0 ? a.length() : pa + 1;
    ib = pb < 0 ? b.length() : pb + 1;
  }
  return 0;
}

const String& aggVersioneNuova() { return versioneNuova; }

EsitoControllo aggControlla() {
  if (WiFi.status() != WL_CONNECTED) return AGG_ERRORE;
  WiFiClientSecure c;
  c.setCACert(CERT_GITHUB);
  HTTPClient h;
  h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  h.setTimeout(8000);
  if (!h.begin(c, AGG_URL)) return AGG_ERRORE;
  if (h.GET() != 200) { h.end(); return AGG_ERRORE; }
  JsonDocument d;
  DeserializationError err = deserializeJson(d, h.getString());
  h.end();
  if (err) return AGG_ERRORE;
  versioneNuova = (const char*)(d["versione"] | "");
  urlFirmware = (const char*)(d["url"] | "");
  shaAtteso = (const char*)(d["sha256"] | "");
  shaAtteso.toLowerCase();
  if (!versioneNuova.length() || !urlFirmware.length() || shaAtteso.length() != 64) return AGG_ERRORE;
  return confronta(versioneNuova, FW_VERSION) > 0 ? AGG_NUOVO : AGG_GIA_AGGIORNATO;
}

bool aggInstalla() {
  otaSchermata(0, tr(T_NON_SPEGNERE));
  WiFiClientSecure c;
  c.setCACert(CERT_GITHUB);
  HTTPClient h;
  h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  h.setTimeout(15000);
  if (!h.begin(c, urlFirmware)) return false;
  if (h.GET() != 200) { h.end(); return false; }
  int totale = h.getSize();
  if (!Update.begin(totale > 0 ? totale : UPDATE_SIZE_UNKNOWN)) { h.end(); return false; }
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  WiFiClient* s = h.getStreamPtr();
  static uint8_t buf[2048];
  int fatto = 0, ultimo = -1;
  uint32_t fermo = millis();
  while (h.connected() && (totale < 0 || fatto < totale)) {
    size_t n = s->available();
    if (!n) { if (millis() - fermo > 15000) break; delay(2); continue; }
    n = s->readBytes(buf, min(n, sizeof(buf)));
    fermo = millis();
    if (Update.write(buf, n) != n) break;
    mbedtls_sha256_update(&sha, buf, n);
    fatto += n;
    int p = totale > 0 ? (int)(100LL * fatto / totale) : 0;
    if (p != ultimo) { ultimo = p; char t[16]; snprintf(t, sizeof t, "%d %%", p); otaSchermata(p, t); }
  }
  h.end();
  uint8_t dig[32];
  mbedtls_sha256_finish(&sha, dig);
  mbedtls_sha256_free(&sha);
  char hex[65];
  for (int i = 0; i < 32; ++i) sprintf(hex + 2 * i, "%02x", dig[i]);
  if ((totale > 0 && fatto != totale) || shaAtteso != String(hex)) {   // file incompleto o diverso: non si attiva
    Update.abort();
    otaSchermata(0, tr(T_ERRORE_OTA));
    delay(2000);
    return false;
  }
  if (!Update.end(true)) return false;
  otaSchermata(100, tr(T_RIAVVIO));
  delay(800);
  ESP.restart();
  return true;
}
