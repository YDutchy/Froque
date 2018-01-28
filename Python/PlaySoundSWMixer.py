import swmixer
import time
import serial
from timeit import default_timer as timer

arduino = serial.Serial('COM7', 19200)
swmixer.init(samplerate=44100, chunksize=1024, stereo=True)
swmixer.start()

sndMain = swmixer.Sound("Sounds/RelaxedBackgroundShorter.wav")
snd1 = swmixer.Sound("Sounds/Eagle.wav")
snd2 = swmixer.Sound("Sounds/Crow1.wav")
snd3 = swmixer.Sound("Sounds/Lightning.wav")
snd4 = swmixer.Sound("test.wav")

sndArray = [snd1, snd2, snd3]
canPlay = 0
valueArray = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
count = 0
start = 0
mainSound = 0

# sndMain.play(loops=-1, volume=0.0)
while True:
    print "HI"
    totalValue = 0
    arduino.write(str(chr(33)))
    for i in range(0, 15):
        value = arduino.readline()[:-2]
        if value:
            print value
            valueArray[i] = int(value)
            totalValue += int(value)
    # print totalValue
    if 2 in valueArray:
        if canPlay == 0:
            start = timer()
            canPlay = 1
            count = 0
            mainSound = sndMain.play(loops=-1, volume=0.5)

    print "ValueArray: ", valueArray, "Can Play: ", canPlay, "Total Value: ", totalValue
    # Sound is allowed, run the scripts from here
    if canPlay == 1:
        end = timer()
        if (end - start) > 300:
            canPlay = 0
        if totalValue > 5:
            snd2.play(volume=0.3)
            time.sleep(10)
            snd1.play(volume=0.2)
            time.sleep(10)
        elif totalValue > 2:
            snd1.play(volume=0.5)
            time.sleep(10)
        time.sleep(1)
        print "LOL"
        time.sleep(5)

    # Sound is not allowed
    #else:
    if 2 not in valueArray:
        count += 1
        if count > 5:
            canPlay = 0
            count = 0
            mainSound.stop()
        time.sleep(5)
