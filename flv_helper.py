from flv import *
from ctypes import *

### Wrappers to create flv construct objects

def flvheader_create():
    return flv_header.build(Container(Signature = "FLV", Version = 0,
                                      TypeFlags = 0, DataOffset = 0))



def flvtag_create(prevTagSize, tag):
    return Container(
        TypeRes=Container(Reserved=0, Filter=0, TagType=18),
        BodySize=Container(BodySize=0),
        Timestamp=Container(Timestamp=0),
        TimestampExtended=0,
        StreamID=Container(StreamID=0),
        Tag=tag
        )

def scriptdata_create(name):
    return Container(NameType=2, Name=name, ArrayType=8,
                     Length=0, ScriptdataProperty=[],
                     ObjectTerminator1=0,
                     ObjectTerminator2=0,
                     ObjectTerminator3=9)

def scriptdata_propstr(name, prop):
    return Container(Name=name, Type=2, Body=prop)
    
def scriptdata_addpropstr(sd, name, str):
    prop = scriptdata_propstr(name, str)
    sd.Length += 1
    sd.ScriptdataProperty += [prop]
    return sd

def scriptdata_propnum(name, num):
    return Container(Name=name, Type=0, Body=num)

def scriptdata_addpropnum(sd, name, num): # TODO collapse scriptdata_addprop to one function
    prop = scriptdata_propnum(name, num)
    sd.Length += 1
    sd.ScriptdataProperty += [prop]
    return sd

### Wrapper functions for recursive arrays

def scriptdata_addproparr(sd, name, arr):
    sd.Length += 1
    sd.ScriptdataProperty += [arr]
    return sd

def scriptdata_proparr(name):
    return Container(Name=name, Type=8,
                     Body=Container(Length=0,
                                    ScriptdataProperty=[],
                                    ObjectTerminator1=0,
                                    ObjectTerminator2=0,
                                    ObjectTerminator3=9))

def scriptdata_arradd(arr, prop):
    arr.Body.Length += 1
    arr.Body.ScriptdataProperty += [prop]

### Testing functions

def cue_gen():
    sd = scriptdata_create("onCuePoint")

    scriptdata_addpropstr(sd, "name", "Mysecondcuepoint")
    scriptdata_addpropnum(sd, "time", 0)
    scriptdata_addpropstr(sd, "type", "navigation")
    par1 = scriptdata_propstr("lights", "beginning")
    
    arr = scriptdata_proparr("parameters")
    scriptdata_arradd(arr, par1)
    scriptdata_addproparr(sd, "parameters", arr)

    return sd

def so_fixsize(ft):
    s = len(flv_tag.build(ft))
    ft.BodySize.BodySize=s-11
    return s

def read_cue(filename):
    f = open(filename).read()
    print flv.parse(f)

def cuecumber_insert():
    sd = cue_gen()
    ft = flvtag_create(0, sd)
    prevsize = so_fixsize(ft)
    ftt = flv_tag.build(ft)

    f = open('cuepoint_gen.raw', 'w')
    f.write(ftt)

    libcuecumber = cdll.LoadLibrary("./libcuecumber.so")
    libcuecumber.cuecumber_init()
    libcuecumber.insert_cuepoint(prevsize, ftt)
    libcuecumber.cuecumber_exit()


### Emcee function wrappers

def cuecumber_startstream():
    global libcuecumber
    libcuecumber = cdll.LoadLibrary("./libcuecumber.so")
    libcuecumber.cuecumber_init()

def cuecumber_stopstream():
    libcuecumber.cuecumber_exit()


def cuecumber_changeslide(slide_number):
    sd = cuecumber_generate_slidecuepoint(slide_number)
    ft = flvtag_create(0, sd)
    prevsize = so_fixsize(ft)
    ftt = flv_tag.build(ft)

    libcuecumber.insert_cuepoint(prevsize, ftt)

def cuecumber_generate_slidecuepoint(slide_number):
    sd = scriptdata_create("onCuePoint")

    scriptdata_addpropstr(sd, "name", "Mysecondcuepoint")
    scriptdata_addpropnum(sd, "time", 0)
    scriptdata_addpropnum(sd, "slide", slide_number)
    scriptdata_addpropstr(sd, "type", "navigation")
    par1 = scriptdata_propstr("lights", "beginning") 
   
    arr = scriptdata_proparr("parameters")
    scriptdata_arradd(arr, par1)
    scriptdata_addproparr(sd, "parameters", arr)

    return sd


#read_cue('cuepoints.flv')
#cuecumber_insert()
