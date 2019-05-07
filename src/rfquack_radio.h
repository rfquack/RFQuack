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

#ifndef rfquack_radio_h
#define rfquack_radio_h

#include "rfquack_common.h"
#include "rfquack_logging.h"
#include "rfquack_transport.h"

#include "rfquack.pb.h"

#include <cppQueue.h>

/*****************************************************************************
 * Variables
 *****************************************************************************/
/* TX and RX queues */
Queue rfquack_tx_q(sizeof(rfquack_Packet), RFQUACK_RADIO_TX_QUEUE_LEN, FIFO,
                   true);

Queue rfquack_rx_q(sizeof(rfquack_Packet), RFQUACK_RADIO_RX_QUEUE_LEN, FIFO,
                   true);

/* Radio instance */
RFQRadio rfquack_rf(RFQUACK_RADIO_PIN_CS, RFQUACK_RADIO_PIN_RST,
                    RFQUACK_RADIO_PIN_IRQ);

/**
 * @brief Changes the radio state in the radio driver.
 */
void rfquack_update_mode() {
  if (rfq.mode == rfquack_Mode_IDLE)
    rfquack_rf.setModeIdle();

  if (rfq.mode == rfquack_Mode_REPEAT || rfq.mode == rfquack_Mode_RX)
    rfquack_rf.setModeRx();

  // never set TX mode, it happens automatically
}

/**
 * @brief Changes preamble length in the radio driver.
 */
void rfquack_update_preamble() {
  rfquack_rf.setPreambleLength(rfq.modemConfig.preambleLen);
  Log.trace("Preamble length:     %d bytes", rfq.modemConfig.preambleLen);
}

/**
 * @brief Changes frequency in the radio driver.
 */
void rfquack_update_frequency() {
  if (!rfquack_rf.setFrequency(rfq.modemConfig.carrierFreq)) {
    Log.error("⚠️  Set frequency failed");
    return;
  }
}

/**
 * @brief Changes TX power in the radio driver
 */
void rfquack_update_tx_power() {
    rfquack_rf.setTxPower(rfq.modemConfig.txPower
#ifdef RFQUACK_RADIO_SET_HIGHPOWER
        ,rfq.modemConfig.isHighPowerModule
#endif
        );
}

/**
 * @brief Chages modem config choice in the radio driver
 */
void rfquack_update_modem_config_choice() {
  rfquack_rf.setModemConfig(
      (RFQRadioModemConfigChoice)rfq.modemConfig.modemConfigChoiceIndex);

  Log.trace("Modem config set to %d", rfq.modemConfig.modemConfigChoiceIndex);
}

/**
 * @brief Changes sync words in the radio driver.
 */
void rfquack_update_sync_words() {
  if (rfq.modemConfig.syncWords.size > 0) {
    rfquack_rf.setSyncWords((uint8_t *)(rfq.modemConfig.syncWords.bytes),
                            rfq.modemConfig.syncWords.size);
  } else {
    Log.trace("Sync Words:          None (sync words detection disabled)");
    uint8_t nil = 0;
    rfquack_rf.setSyncWords(&nil, rfq.modemConfig.syncWords.size);
  }
}

/*
 * Setup the radio:
 *
 *      - initialize the driver
 *      - set the carrier frequency
 *      - set the modem registers
 *      - set preamble length and sync words
 */
void rfquack_radio_setup() {
  rfquack_rf.setDebug(RFQUACK_DEBUG_RADIO);
  rfquack_rf.setPrinter(&LogPrinter);

  Log.trace("📡 Setting up radio (CS: %d, RST: %d, IRQ: %d)",
            RFQUACK_RADIO_PIN_CS, RFQUACK_RADIO_PIN_RST, RFQUACK_RADIO_PIN_IRQ);

  if (!rfquack_rf.init()) {
    Log.error("⚠️  Radio init failed");
    return;
  }

  Log.trace("📶 Radio initialized (debugging: %T)", RFQUACK_DEBUG_RADIO);

#ifdef RFQUACK_RADIO_SET_FREQ
  rfquack_update_frequency();
#endif

#ifdef RFQUACK_RADIO_PARTNO
  Log.trace(RFQUACK_RADIO " type %X ready to party 🎉", rfquack_rf.deviceType());
#endif

#ifdef RFQUACK_RADIO_SET_POWER
  rfquack_update_tx_power();
#endif

#ifdef RFQUACK_RADIO_SET_RF
    // TODO NRF24/51/73 call .setRF(datarate, power)
#endif

#ifdef RFQUACK_RADIO_HAS_MODEM_CONFIG
  rfquack_update_modem_config_choice();

  rfquack_update_sync_words();
#ifdef RFQUACK_RADIO_SET_PREAMBLE
  rfquack_update_preamble();
#endif

  Log.trace("Max payload length:  %d bytes", RFQUACK_RADIO_MAX_MSG_LEN);
#endif // RFQUACK_RADIO_HAS_MODEM_CONFIG

  rfquack_update_mode();

  Log.trace("📶 Radio is fully set up (RFQuack mode: %d, radio mode: %d)",
            rfq.mode, rfquack_rf.mode());
}

/**
 * @brief Update queue statistics.
 */
void rfquack_update_radio_stats() {
  rfq.stats.tx_queue = rfquack_tx_q.getCount();
  rfq.stats.rx_queue = rfquack_rx_q.getCount();
}

/**
 * @brief Enqueue a packet in a given queue.
 *
 * Check if the packet holds no more than the RFQUACK_RADIO_MAX_MSG_LEN bytes,
 * check if the queue has enough room available, and push the packet into it.
 *
 * @param q Pointer to queue
 * @param pkt Packet to be enqueued
 *
 * @return True only if the queue has room and enqueueing went through.
 */
bool rfquack_enqueue_packet(Queue *q, rfquack_Packet *pkt) {
  if (pkt->data.size > RFQUACK_RADIO_MAX_MSG_LEN) {
    Log.error("Cannot enqueue: message length exceeds limit");
    return false;
  }

  if (q->isFull()) {
    Log.error("Cannot enqueue because queue is full:"
              " slow down or increase the queue size!");
    return false;
  }

  q->push(pkt);

  Log.verbose("Packet enqueued: %d bytes", pkt->data.size);

  return true;
}

/**
 * @brief Transmit a packet in the air.
 *
 * Fill a packet buffer with data up to the lenght, set its size to len, and
 * enqueue it on the TX queue for transmission.
 *
 * @param data Pointer to a buffer containing the bytes to be transmitted
 * @param len Number of bytes to copy from the buffer and transmit
 * @param repeat Transmission repetitions (0, default, means no transmissions)
 *
 * @return Number of packets enqueued correctly.
 */
uint32_t rfquack_send_packet(uint8_t *data, uint32_t len, uint32_t repeat) {
  if (repeat == 0)
    return 0;

  if (len < RFQUACK_RADIO_MIN_MSG_LEN || len > RFQUACK_RADIO_MAX_MSG_LEN) {
    Log.error("Payload length must be within %d and %d bytes",
              RFQUACK_RADIO_MIN_MSG_LEN, RFQUACK_RADIO_MAX_MSG_LEN);
    return 0;
  }

  uint32_t correct = 0;

  rfquack_Packet pkt;
  memcpy(pkt.data.bytes, data, len);
  pkt.data.size = len;
  pkt.millis = millis();
  pkt.has_millis = true;

  for (uint32_t i = 0; i < repeat; i++)
    correct += (uint32_t)rfquack_enqueue_packet(&rfquack_tx_q, &pkt);

  return correct;
}

/**
 * @brief Transmit a packet in the air.
 *
 * Fill a packet buffer with data up to the lenght, set its size to len, and
 * enqueue it on the TX queue for transmission.
 *
 * @param data Pointer to a buffer containing the bytes to be transmitted
 * @param len Number of bytes to copy from the buffer and transmit
 *
 * @return True only if the data has been enqueued correctly.
 */
bool rfquack_send_packet(uint8_t *data, uint32_t len) {
  return rfquack_send_packet(data, len, 1) != 1;
}

/**
 * @brief If any packets are in RX queue, send them all via the transport.
 * If in repeat mode, modify and retransmit.
 *
 * We send them one at a time, even if there are more than one in the queue.
 * This is because we rather have a full RX queue but zero packet lost, and
 * we want to give other functions in the loop their share of time.
 */
void rfquack_rx_flush_loop() {
  rfquack_Packet pkt;
  while (rfquack_rx_q.pop(&pkt)) {
    if (rfq.mode == rfquack_Mode_REPEAT) {
      // apply all packet modifications
      rfquack_apply_packet_modifications(&pkt);

      // send
      rfquack_send_packet(pkt.data.bytes, pkt.data.size, rfq.tx_repeat_default);

      return;
    }

    uint8_t buf[RFQUACK_MAX_PB_MSG_SIZE];
    pb_ostream_t ostream = pb_ostream_from_buffer(buf, sizeof(buf));

    if (!pb_encode(&ostream, rfquack_Packet_fields, &pkt)) {
      Log.error("Encoding failed: %s", PB_GET_ERROR(&ostream));
    } else {
      if (!rfquack_transport_send(
            RFQUACK_OUT_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PACKET, buf,
            ostream.bytes_written))
        Log.error("Failed sending to transport");
      else
        Log.verbose("%d bytes of data sent on the transport",
            ostream.bytes_written);
    }
  }
}

/*
 * If any packets are in TX queue, send one of them.
 *
 * TODO decide if we just send them all in burst
 */
void rfquack_tx_flush_loop() {
  rfquack_Packet pkt;

  while (rfquack_tx_q.pop(&pkt))
    if (!rfquack_rf.send(pkt.data.bytes, pkt.data.size))
      Log.error("Cannot TX: radio driver failure");
    else
      rfq.stats.tx_packets++;
}

/**
 * @brief Reads any data from the RX FIFO and push it to the RX queue.
 *
 * This is the main receive loop.
 */
void rfquack_rx_loop() {
  if (rfq.mode != rfquack_Mode_RX && rfq.mode != rfquack_Mode_REPEAT)
    return;

  rfquack_Packet pkt;
  uint8_t len = sizeof(pkt.data.bytes);

  if (rfquack_rf.available()) { // this calls RH_*::setModeRx() internally
    if (rfquack_rf.recv((uint8_t *)pkt.data.bytes, &len)) {
      if (len > 0 && len <= RFQUACK_RADIO_MAX_MSG_LEN) {

#ifdef RFQUACK_DEV
        Log.verbose("Data size = %d (bounds: %d, %d)", len, 0,
                    RFQUACK_RADIO_MAX_MSG_LEN);
#endif

        pkt.data.size = len;
        pkt.millis = millis();
        pkt.has_millis = true;

        if (rfquack_packet_filter(&pkt)) {
          rfquack_enqueue_packet(&rfquack_rx_q, &pkt);

#ifdef RFQUACK_DEV
          rfquack_log_packet(&pkt);
#endif
        }
      }

      rfq.stats.rx_packets++;
    } else
      rfq.stats.rx_failures++;
  }
}

/*
 * Change modem config choice
 */
void rfquack_change_modem_config_choice(uint32_t index) {

  // TODO input validation

  Log.trace("Modem config choice index: %d -> %d",
            rfq.modemConfig.modemConfigChoiceIndex, index);

  rfq.modemConfig.modemConfigChoiceIndex = index;
  rfquack_update_modem_config_choice();
}

/**
 * @brief Read register value (if permitted by the radio driver).
 *
 * @param addr Address of the register (check RadioHead).
 *
 * @return Value from the register.
 */
rfquack_register_value_t rfquack_read_register(rfquack_register_address_t reg) {
  return rfquack_rf.spiRead(reg);
}

/**
 * @brief Write register value (if permitted by the radio driver).
 *
 * @param addr Address of the register (check RadioHead).
 *
 * @return Some devices return a status byte during the first data transfer.
 * This byte is returned. it may or may not be meaningfule depending on the the
 * type of device being accessed (check RadioHead).
 */
uint8_t rfquack_write_register(rfquack_register_address_t reg,
                               rfquack_register_value_t value) {
  return rfquack_rf.spiWrite(reg, value);
}

/*
 * Change TX power
 */
void rfquack_change_tx_power(uint32_t txPower) {

  // TODO input validation

  Log.trace("TX power: %d -> %d", rfq.modemConfig.txPower, txPower);

  rfq.modemConfig.txPower = txPower;
  rfquack_update_tx_power();
}

/*
 * Change sync words
 */
void rfquack_change_sync_words(rfquack_ModemConfig_syncWords_t syncWords) {
  // input validation
  if (syncWords.size == 0) {
    rfq.modemConfig.syncWords.size = 0;
    Log.trace("Disabling sync words detection");
    return;
  } else {
    if (syncWords.size > RFQUACK_MAX_SYNC_WORDS_LEN ||
        syncWords.size < RFQUACK_MIN_SYNC_WORDS_LEN) {
      Log.warning("Sync words must either be NULL, or between %d and %d bytes",
                  RFQUACK_MIN_SYNC_WORDS_LEN, RFQUACK_MAX_SYNC_WORDS_LEN);
      return;
    }
  }

  for (uint8_t i = 0; i < syncWords.size; i++)
    if (syncWords.bytes[i] == 0x00) {
      Log.warning("syncWords[%d] = 0x00, which is disallowed. Not changing!",
                  i);
      return;
    }

  memcpy(rfq.modemConfig.syncWords.bytes, syncWords.bytes, syncWords.size);
  rfq.modemConfig.syncWords.size = syncWords.size;
  rfquack_update_sync_words();
}

/*
 * Change preamble len
 */
void rfquack_change_preamble_len(size_t preambleLen) {
  // TODO input validation

  Log.trace("Preamble length: %d -> %d", rfq.modemConfig.preambleLen,
            preambleLen);

  rfq.modemConfig.preambleLen = preambleLen;
  rfquack_update_preamble();
}

/*
 * Change is high power module
 */
void rfquack_change_is_high_power_module(bool isHighPowerModule) {
  // TODO input validation

  Log.trace("Is high power: %d -> %d", rfq.modemConfig.isHighPowerModule,
            isHighPowerModule);

  rfq.modemConfig.isHighPowerModule = isHighPowerModule;
  rfquack_update_tx_power();
}

/*
 * Change carrier frequency
 */
void rfquack_change_carrier_freq(float carrierFreq) {
  // TODO input validation

  Log.trace("Carrier frequency: %F -> %F", rfq.modemConfig.carrierFreq,
            carrierFreq);

  rfq.modemConfig.carrierFreq = carrierFreq;
  rfquack_update_frequency();
}

uint32_t rfquack_set_modem_config(rfquack_ModemConfig *modemConfig) {
  rfquack_ModemConfig _c = *modemConfig;
  uint32_t changes = 0;

  Log.trace("Changing modem configuration");

  if (_c.has_carrierFreq) {
    rfquack_change_carrier_freq(_c.carrierFreq);
    changes++;
  }

  if (_c.has_txPower) {
    rfquack_change_tx_power(modemConfig->txPower);
    changes++;
  }

  if (_c.has_isHighPowerModule) {
    rfquack_change_is_high_power_module(_c.isHighPowerModule);
    changes++;
  }

  if (_c.has_preambleLen) {
    rfquack_change_preamble_len(_c.preambleLen);
    changes++;
  }

  if (_c.has_syncWords) {
    rfquack_change_sync_words(_c.syncWords);
    changes++;
  }

  if (_c.has_modemConfigChoiceIndex) {
    rfquack_change_modem_config_choice(modemConfig->modemConfigChoiceIndex);
    changes++;
  }

  return changes;
}

#endif
