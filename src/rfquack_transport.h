/*
 * RFQuack is a versatile RF-hacking tool that allows you to sniff, analyze, and
 * transmit data over the air. Consider it as the modular version of the great
 *
 * Copyright (C) 2019 Trend Micro Incorporated
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef rfquack_transport_h
#define rfquack_transport_h

#include "rfquack_common.h"
#include "rfquack_network.h"
#include "modules/ModulesDispatcher.h"

extern ModulesDispatcher modulesDispatcher;

/**
 * Called every time there's an inbound message from the other side.
 * 
 * @param topic Message topic.
 * @param payload Message payload.
 * @param payload_length Message length.
 */
void rfquack_transport_recv(char *topic, uint8_t *payload, uint32_t payload_length) {
  // Topic example: rfquack/in/{ <verb> : [set|get|info] }/<module_name>/<args>/<args>/<args>/...

  // Split topic into single tokens.
  char *moduleName = NULL;  // <module_name>
  char *verb = NULL;        // <verb>
  char *args[RFQUACK_TOPIC_MAX_TOPIC_ARGS] = {NULL}; // Array of 0 or more pointers to args.
  uint8_t argsLen = 0; // Len of 'args' array.

  // Start tokenizing.
  char *token = strtok(topic, RFQUACK_TOPIC_SEP);
  uint8_t tokenOrdinal = 0;
  while (token != NULL) {

    // Save pointers to each token and perform basic sanity checks.
    switch (tokenOrdinal) {
      case 0:
        // Short circuit if topic prefix is not correct.
        if (strncmp(topic, RFQUACK_TOPIC_PREFIX, strlen(RFQUACK_TOPIC_PREFIX)) != 0 &&
            strncmp(topic, RFQUACK_TOPIC_BROADCAST_PREFIX, strlen(RFQUACK_TOPIC_BROADCAST_PREFIX)) != 0) {
#ifdef RFQUACK_DEV
          Log.warning(F("Ignoring message with invalid prefix: %s"), token);
#endif
          return;
        }
        break;
      case 1:
        // Short circuit if direction is not correct (must be IN)
        if (strncmp(token, RFQUACK_TOPIC_IN, strlen(RFQUACK_TOPIC_IN)) != 0) {
#ifdef RFQUACK_DEV
          Log.warning(F("Message has wrong direction: %s"), token);
#endif
          return;
        }
      case 2:
        verb = token;
        break;
      case 3:
        moduleName = token;
        break;
    }

    // Store args (if any)
    if (tokenOrdinal >= 4) {
      args[argsLen] = token;
      argsLen++;
    }

    // Go on tokenizing.
    token = strtok(nullptr, RFQUACK_TOPIC_SEP);
    tokenOrdinal++;
  }

  // Exit if the topic is not valid
  if (tokenOrdinal < 3) {
#ifdef RFQUACK_DEV
    Log.warning(F("Topic must have at least 3 tokens"));
#endif
    return;
  }

  // Dispatch received command to modules.
  modulesDispatcher.executeUserCommand(moduleName, verb, args, argsLen, (char *) payload, payload_length);
}

#if defined(RFQUACK_TRANSPORT_MQTT)

#include <MQTT.h>
MQTTClient rfquack_mqtt(RFQUACK_MQTT_MAX_PACKET_SIZE);

/*
 * This is called every time there's an inbound MQTT message from the other
 * side.
 */
static void rfquack_transport_mqtt_recv(MQTTClient *client, char *topic,
                                        char *payload, int payload_length) {
  rfquack_transport_recv(topic, (uint8_t *)payload, (uint32_t)payload_length);
}

static void rfquack_mqtt_connect() {
  if (rfquack_mqtt.connected())
    return;

  String clientId = RFQUACK_UNIQ_ID;

#ifdef RFQUACK_DEV
  Log.trace("Connecting %s to MQTT broker %s:%d", clientId.c_str(),
            RFQUACK_MQTT_BROKER_HOST, RFQUACK_MQTT_BROKER_PORT);
#endif

  while (!rfquack_mqtt.connected()) {
    delay(RFQUACK_MQTT_RETRY_DELAY);

    if (!rfquack_mqtt.connect(clientId.c_str()
#if defined(RFQUACK_MQTT_BROKER_USER)
                                  ,
                              RFQUACK_MQTT_BROKER_USER
#endif
#if defined(RFQUACK_MQTT_BROKER_PASS)
                              ,
                              RFQUACK_MQTT_BROKER_PASS
#endif
                              )) {
      Log.warning("MQTT error = %d, return = %d", rfquack_mqtt.lastError(),
                  rfquack_mqtt.returnCode());
    }
  }

  Log.trace("MQTT connected");

  while (!rfquack_mqtt.subscribe(RFQUACK_IN_TOPIC_WILDCARD)) {
    Log.error("Failure subscribing to topic: %s", RFQUACK_IN_TOPIC_WILDCARD);

    delay(RFQUACK_MQTT_RETRY_DELAY);
  }

  Log.trace("Subscribed to topic: %s", RFQUACK_IN_TOPIC_WILDCARD);

  while (!rfquack_mqtt.subscribe(RFQUACK_IN_BROADCAST_TOPIC_WILDCARD)) {
    Log.error("Failure subscribing to topic: %s", RFQUACK_IN_BROADCAST_TOPIC_WILDCARD);

    delay(RFQUACK_MQTT_RETRY_DELAY);
  }

  Log.trace("Subscribed to topic: %s", RFQUACK_IN_BROADCAST_TOPIC_WILDCARD);
}

/**
 * Connect transport (MQTT) and keep retrying every 5s if failure
 */
void rfquack_transport_loop() {
  if (!rfquack_mqtt.connected()) {
    Log.warning("MQTT transport not connected");

    rfquack_mqtt_connect();
  }

  rfquack_mqtt.loop();
}

void rfquack_transport_connect() { rfquack_mqtt_connect(); }

void rfquack_transport_setup() {
  Client *rfquack_net = rfquack_network_client();
  
  rfquack_mqtt.begin(RFQUACK_MQTT_BROKER_HOST,
#if defined(RFQUACK_MQTT_BROKER_PORT)
                     RFQUACK_MQTT_BROKER_PORT,
#endif
                     *rfquack_net);
  rfquack_mqtt.setOptions(RFQUACK_MQTT_KEEPALIVE, RFQUACK_MQTT_CLEAN_SESSION,
                          RFQUACK_MQTT_SOCKET_TIMEOUT);
  rfquack_mqtt.onMessageAdvanced(rfquack_transport_mqtt_recv);
}

uint32_t rfquack_transport_send(const char *topic, const uint8_t *data,
                            uint32_t len) {
  Log.trace("Transport is sending %d bytes on topic %s", len, topic);

  if (rfquack_mqtt.publish(topic, (char *) data, len))
    return len;

  return 0;
}

#elif defined(RFQUACK_TRANSPORT_SERIAL)

#include <base64.hpp>

// topic
uint8_t rfquack_topic_buf[RFQUACK_MAX_TOPIC_LEN];
uint32_t rfquack_topic_buf_len = 0;

// payload
uint8_t rfquack_payload_buf[RFQUACK_SERIAL_B64_MAX_PACKET_SIZE];
uint32_t rfquack_payload_buf_len = 0;

// state
uint8_t rfquack_serial_receiving = 0;
bool rfquack_serial_data_ready = false;

void rfquack_transport_connect() {
  Serial.begin(RFQUACK_SERIAL_BAUD_RATE);

  while (!Serial);

  Log.trace("Serial transport connected");
}

void rfquack_transport_setup() {
  Log.trace("Setting up serial transport");
  rfquack_transport_connect();
}

/**
 * @brief Send data over serial transport.
 *
 * This turns out to be very slow. I tried to set a baud rate > 9600, but the
 * Python client misses it.
 *
 * @param topic Topic of the message
 * @param data Payload of the message
 * @param len Length of the payload
 *
 * @return Number of bytes actually written over serial
 */
uint32_t rfquack_transport_send(const char *topic, const uint8_t *data,
                                uint32_t len) {

  uint32_t written = 0;

  Log.trace("Transport is sending %d bytes on topic %s", len, topic);

  written += Serial.write((uint8_t) RFQUACK_SERIAL_PREFIX_OUT_CHAR);
  written += Serial.write((uint8_t *) topic, strlen(topic));
  written += Serial.write((uint8_t) RFQUACK_SERIAL_TOPIC_DATA_SEPARATOR_CHAR);

  // encode payload in Base64
  uint8_t enc_data[RFQUACK_SERIAL_B64_MAX_PACKET_SIZE];
  uint32_t enc_len = encode_base64(data, len, enc_data);

  // send payload
  written += Serial.write(enc_data, enc_len);
  written += Serial.write((uint8_t) RFQUACK_SERIAL_SUFFIX_OUT_CHAR);

  return written >= len;
}

/**
 * @brief Processes chars from the serial port and looks for prefix-suffix
 * enclosed data.
 */
static void rfquack_serial_recv() {
  char rc;

  // Note that `rfquack_serial_data_ready` means that we haven't read
  // prefix + data + suffix
  while (Serial.available() > 0 && !rfquack_serial_data_ready) {
    // read one char
    rc = Serial.read();

    if (rc == RFQUACK_SERIAL_PREFIX_IN_CHAR &&
        rfquack_serial_receiving == 0) {        // 0 == waiting for preamble
      rfquack_serial_receiving = 1;             // receiving topic
    } else if (rfquack_serial_receiving == 1) { // 1 == receiving topic
      if (rc == RFQUACK_SERIAL_TOPIC_DATA_SEPARATOR_CHAR) {
        rfquack_serial_receiving = 2; // receiving payload
        rfquack_topic_buf[rfquack_topic_buf_len] = '\0';
      } else
        rfquack_topic_buf[rfquack_topic_buf_len++] = rc;
    } else if (rfquack_serial_receiving == 2) {
      if (rc == RFQUACK_SERIAL_SUFFIX_IN_CHAR) {
        rfquack_serial_data_ready = true;
      } else
        rfquack_payload_buf[rfquack_payload_buf_len++] = rc;
    }
  }
}

/**
 * Run RFQuack serial loop.
 * 
 */
void rfquack_transport_loop() {
  rfquack_serial_recv();

  if (rfquack_serial_data_ready) {
    // copy topic to free the buffer
    char topic[RFQUACK_MAX_TOPIC_LEN];
    memcpy(topic, rfquack_topic_buf, rfquack_topic_buf_len + 1);

    // decode the payload
    uint8_t payload[RFQUACK_SERIAL_MAX_PACKET_SIZE];
    uint32_t payload_length = decode_base64(
      rfquack_payload_buf, rfquack_payload_buf_len, payload);

    // free the buffers and unlock receive function
    rfquack_topic_buf_len = 0;
    rfquack_payload_buf_len = 0;
    rfquack_serial_receiving = 0; // unlock

    // send data to the transport callback
    rfquack_transport_recv(topic, (uint8_t *) payload, payload_length);

    // make recv buffer available
    rfquack_serial_data_ready = false;
  }
}

#else

#error "You must choose one transport type by defining RFQUACK_TRANSPORT_*"

#endif

#endif
