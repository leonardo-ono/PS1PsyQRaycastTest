import math

#print("hello")

for y in range(239, 160, -1):
    d = y - 120
    a = math.atan(d / 160)
    z = 32 / math.tan(a) 
    #print("y=", y, " z=", z)
    print("floorAngle[", y, "] = ", int(z), ";")