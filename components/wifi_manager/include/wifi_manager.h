#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa la pila Wi-Fi en modo estación (STA), conecta a la red
 *        indicada y bloquea hasta obtener una IP.
 *
 * @param ssid      SSID de la red Wi-Fi.
 * @param password  Contraseña de la red Wi-Fi.
 */
void wifi_manager_init_sta(const char *ssid, const char *password);

#ifdef __cplusplus
}
#endif
