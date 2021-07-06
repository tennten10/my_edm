# SudoChef Board code

This code is configured for 3 versions of prototypes at the moment.

V1 is a separate base design with a cutting surface attachable to the top. It uses half of the lcd screen to create a skinny pass-through display.
V3 is a flipable design with a projection type display
V6 is a one sided version using the majority of an lcd screen passing through one layer of wood. 

Configuration takes place by running MenuConfig, and in the a_config file. Pin defs are done in separate files. 

If updating the data/config.txt or data/calib.txt, config needs to end on a comma at the end of a populated line. calib needs to end on an empty new line

Eigen is installed by just copying the folder with the header files. Had some issues when installing through Git, so this method is just easier and it shouldn't change that much in the future. 

lvgl esp32 drivers are cloned in my own repo with some parts disabled and display register offsets modified to be defined specifically to the parts I'm using. 

Need to flash the NVS fctry.csv file separately. Follow directions on https://medium.com/the-esp-journal/building-products-creating-unique-factory-data-images-3f642832a7a3
the functions I ran from esp-idf command prompt that worked are

1. ->>>     C:\Users\parr3\esp\esp-idf\components\nvs_flash\nvs_partition_generator> python nvs_partition_gen.py generate C:\Users\parr3\Documents\PlatformIO\Projects\SudoBoard\fctry.csv C:\Users\parr3\Documents\PlatformIO\Projects\SudoBoard\fctry.bin 0x4000

2. ->>>     C:\Users\parr3\esp\esp-idf\components\esptool_py\esptool> esptool.py --port COM5 write_flash 0xE76000 C:\Users\parr3\Documents\PlatformIO\Projects\SudoBoard\fctry.bin

Then pressing reset/prog buttons like normal to upload to the board

Bluetooth Connections and Functions:
--------------------------------------
Set Wifi:
Enter SSID first in SSID field with UUID "0000551d-60be-11eb-ae93-0242ac130002"
This saves it temporarily in a global variable...
Enter the password in UUID "0000fa55-60be-11eb-ae93-0242ac130002" 
When the password is entered it verifies the ssid/pass combination and either saves them or throws the combo out. If the connection is unsuccessful, you need to re-enter the ssid followed by the password.

Ok to complicate this further, BLE can only handle messages of 20 bytes. So any longer SSIDs and passwords need to be read/written in parts and put together later. Some people have used Notify for longer messages, but I'll just use successive writes to the same characteristic for each split. The splitting needs to be handled on the connecting device before passing it along to the board. Each message is 20 bytes long unless it is the last one. To signal that the entire message is sent, the end characters "]$" need to be at the end of the message. If they would be split into separate messages, they should be both moved into the second message. Probably should check character validity and length on both ends. Not going to implement this now since it'll be interesting to call back to the sending device whether it was accepted. 
End sequence will be "]$" since they are not allowed in ssids and discouraged/not used in passwords... I think. 

When switching versions, need to set in SUDOBOARD menu and LVGL TFT menu