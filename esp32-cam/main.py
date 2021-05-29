from machine import Pin, I2C; i2c = I2C(0, scl=Pin(19), sda=Pin(22), freq=100000)

i2c.scan(); # wake the attiny
i2c.writeto(8, 'v'); # ask to read the voltage
print(i2c.readfrom(8, 5)); # read the voltage

i2c.scan(); # just in case
i2c.writeto(8, 's'); # go to sleep
