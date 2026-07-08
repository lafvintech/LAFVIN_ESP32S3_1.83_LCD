.. _install_local_library:

Install Local Libraries
=========================

The source code package includes the required Arduino libraries as ``.zip`` files in the ``lib/`` folder, so you can install them offline without downloading from the Library Manager.

Provided Libraries
------------------

The following libraries are included:

.. list-table::
   :header-rows: 1
   :widths: 30 30

   * - ZIP File
     - Library
   * - ``ArduinoJson.zip``
     - ArduinoJson Library
   * - ``lvgl.zip``
     - LVGL Graphics Library
   * - ``SensorLib.zip``
     - Sensor Library
   * - ``TFT_eSPI.zip``
     - TFT LCD Display Driver

Method 1: Install via Arduino IDE (Recommended)
------------------------------------------------

#. Open Arduino IDE.

#. Go to **Sketch > Include Library > Add .ZIP Library...**

  .. image:: img/lib_ins1.png

#. In the file browser, navigate to the ``lib/`` folder in the source code package.

#. Select a ``.zip`` file (e.g., ``lvgl.zip``) and click **Open**.

    .. image:: img/lib_ins2.png

#. Arduino IDE will extract and install the library automatically. A notification **"Library installed"** will appear in the bottom status bar.

    .. image:: img/lib_ins3.png

    .. image:: img/lib_ins4.png

#. Repeat steps 2–5 for each of the remaining ``.zip`` files.

After installation, you can verify the libraries are installed by going to **Sketch > Include Library** — they should appear at the bottom of the list under "Contributed libraries".

Method 2: Manual Installation
-------------------------------

If the ZIP method does not work, you can install libraries manually:

#. Locate your Arduino libraries folder:

   * **Windows**: ``C:\Users\<YourName>\Documents\Arduino\libraries\``
   * **macOS**: ``~/Documents/Arduino/libraries/``
   * **Linux**: ``~/Arduino/libraries/``

#. Extract each ``.zip`` file into the ``libraries/`` folder.

   For example, after extracting ``lvgl.zip``, you should have::

      Arduino/
      └── libraries/
          └── lvgl/
              ├── library.properties
              ├── src/
              └── ...

#. Restart Arduino IDE, and the libraries will be recognized automatically.

Verify Installation
-------------------

To check if all libraries are installed correctly:

#. Open Arduino IDE and go to **File > Examples**.

#. Scroll down to the "Examples from custom libraries" section.

#. You should see entries for **LVGL**, **TFT_eSPI**, **ArduinoJson**, and **SensorLib**.

If any library is missing, repeat the installation for that specific ``.zip`` file.
