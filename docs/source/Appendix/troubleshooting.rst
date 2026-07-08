.. _troubleshooting:

Troubleshooting
===============

Use this page to collect common setup and runtime problems.

Board Is Not Detected
=====================

* **Symptom**: No serial port appears after connecting the board.
* **Possible Causes**:

  1. Charging-only USB cable.
  2. Missing USB serial driver.
  3. Board is not powered or is connected through an unstable hub.

* **Solutions**:

  1. Use a USB Type-C data cable.
  2. Install the driver by following :ref:`preparation`.
  3. Try another USB port and press **RST**.

Upload Fails
============

* **Symptom**: Arduino IDE or ESP-IDF cannot upload firmware.
* **Possible Causes**:

  1. Wrong board or port selected.
  2. Serial monitor is holding the port.
  3. The board is not in download mode.

* **Solutions**:

  1. Check :ref:`arduino_board_settings`.
  2. Close other serial tools.
  3. Enter download mode and upload again.

Firmware Runs Incorrectly
=========================

* **Symptom**: LCD, QMI8658A sensor, TF card, or wireless behavior does not
  match the tutorial.
* **Possible Causes**:

  1. Wrong firmware image.
  2. Old flash data was not erased.
  3. Required library or TF card is missing.

* **Solutions**:

  1. Reflash the correct firmware with :ref:`online_flasher`.
  2. Enable **Erase Flash** during flashing.
  3. Check the library installation and insert the TF card when the tutorial
     requires storage.
