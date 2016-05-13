"""
python pmonenv.py -f gzrom.bin -o 0x70000 -s 512 al=/dev/mtd0 apend="'root=/dev/mtdblock0'"
"""
import struct
import sys
import getopt
def readenv(fname,foff,fsz,argv):
    f=open(fname,'rb+')
    f.seek(foff,0)
    a=f.read(fsz);
    a=a.ljust(fsz,'\x00')
    f.close()
    d={}
    b = struct.unpack('!'+str(len(a)/2)+'H',a)
    if(sum(b)&0xffff):
     print('checksum error, rebuild env')
     t = argv
    else:
     e = a[2:].find('\x00\x00')
     t = a[2:2+e].split('\x00')+argv
    for i in t:
       a=i.split('=',1)
       if len(a) > 1:
         d[a[0]] = a[1]
       elif d.has_key(a[0]):
         del d[a[0]]
    return d

def writeenv(fname,foff,fsz,d):
    
    a='\x00\x00'
    for i in d.keys():
     a += i+'='+d[i]+'\x00'
    a=a.ljust(fsz,'\x00')
    
    b = struct.pack('!H',(-sum(struct.unpack('!'+str(len(a)/2)+'H',a)))&0xffff)
    a=b+a[2:]
    
    f=open(fname,'rb+')
    f.seek(foff,0)
    f.write(a)
    f.close()
    
if __name__ == '__main__':
    opt,argv=getopt.getopt(sys.argv[1:],'o:s:f:w')
    opt=dict(opt)
    foff = int(opt['-o'],0) if opt.has_key('-o') else 0xb000
    fsz = int(opt['-s'],0) if opt.has_key('-s') else 500
    fname = opt['-f'] if opt.has_key('-f') else 'gzrom.bin'
    
    d=readenv(fname,foff,fsz,argv)
    print(d)
    if opt.has_key('-w'):
     writeenv(fname,foff,fsz,d)

