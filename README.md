# ESP Home

https web server 

configure:
idf.py menuconfig

compile and monitor:
idf.py -p /dev/ttyUSB0 flash monitor

clean:
idf.py fullclean


Create cert:
openssl req -newkey rsa:2048 -nodes -keyout prvkey.pem -x509 -days 365 -out cacert.pem   