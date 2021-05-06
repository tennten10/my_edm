# SudoChef Board code

This code is configured for 3 versions of prototypes at the moment.

V1 is a separate base design with a cutting surface attachable to the top. It uses half of the lcd screen to create a skinny pass-through display.
V3 is a flipable design with a projection type display
V6 is a one sided version using the majority of an lcd screen passing through one layer of wood. 

Configuration takes place by running MenuConfig, and in the a_config file. Pin defs are done in separate files. 

If updating the data/config.txt or data/calib.txt, config needs to end on a comma at the end of a populated line. calib needs to end on an empty new line

Eigen is installed by just copying the folder with the header files. Had some issues when installing through Git, so this method is just easier and it shouldn't change that much in the future. 

lvgl esp32 drivers are cloned in my own repo with some parts disabled and display register offsets modified to be defined specifically to the parts I'm using. 
