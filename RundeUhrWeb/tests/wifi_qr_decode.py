from PIL import Image
import zxingcpp
cases={'wifi_setup':'WIFI:T:WPA;S:RundeUhr-Test;P:AB23456789CD;;','wifi_setup_escaped':'WIFI:T:WPA;S:'+r'\;'*32+';P:AB23456789CD;;'}
for name,expected in cases.items():
    im=Image.open('build/'+name+'.ppm')
    result=zxingcpp.read_barcode(im)
    assert result and result.text==expected,(name,result)
    im.save('build/'+name+'.png')
print('PASS: independent ZXing decoder reads both actual 240x240 rendered QR images, including maximum escaped SSID')
