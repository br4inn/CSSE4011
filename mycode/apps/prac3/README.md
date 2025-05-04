Sniffer 
________________________________________________________________________________
in:
(env)  ~/csse4011/repo/tools/nrfutil$ 

add:
vim ~/.bashrc
export PATH="$HOME/csse4011/repo/tools/nrfutil:$PATH"

run:
cd tools/nrfutil/
nrfutil dfu usb-serial -pkg sniffer.zip -p /dev/ttyACM0


--------------------------------------------------------------------------------
Mobile
________________________________________________________________________________
west build -p -b nrf52840dk/nrf52840 task1