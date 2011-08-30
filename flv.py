from construct import *

def UBInt24(name):
    """unsigned, big endian 24-bit integer"""
    return BitField(name, 24)

def UBInt24m (name):
    """This function utilizes a patched version of pyconstruct, which provides UBInt24() """
    return BitStruct(name, UBInt24(name))

videodata = Struct("VideoData")
audiodata = Struct("AudioData")

flv_tagtype = Enum(Byte("FLV TagType"),
                   Audio = 8,
                   Video = 9,
                   Script_Data = 18,
                   )


scriptdata_prop = Struct("ScriptdataProperty",
                         PascalString("Name", UBInt16("length")),

                         UBInt8("Type"),
                         Switch("Body", lambda ctx: ctx["Type"],
                                {
                                    0 : BFloat64("Value"),
                                    2 : PascalString("Value", UBInt16("length")),
                                    8 : LazyBound("next", lambda: scriptdata_ecmaarray),
                                    },
                                default = Pass
                                )
                         )

scriptdata_ecmaarray = Struct("ScriptdataArray",
                              UBInt32("Length"),
                              MetaRepeater(lambda ctx: ctx["Length"], scriptdata_prop),

                              UBInt8("ObjectTerminator1"),
                              UBInt8("ObjectTerminator2"),
                              UBInt8("ObjectTerminator3"),
                             )


scriptdata = Struct("ScriptData",
                    UBInt8("NameType"),
                    PascalString("Name", UBInt16("length")),
                    UBInt8("ArrayType"),
                    Embed(scriptdata_ecmaarray)
                    )

flv_tag = Struct("Tag",
                 BitStruct("TypeRes",
                           BitField("Reserved", 2),
                           Flag("Filter"),
                           BitField("TagType", 5)
                           ),
                 UBInt24m("BodySize"),
                 UBInt24m("Timestamp"),
                 UBInt8("TimestampExtended"),
                 UBInt24m("StreamID"),
                 
                 Switch("Tag", lambda ctx: ctx["TypeRes"]["TagType"],
                        {
                            8 : audiodata,
                            9 : videodata,
                            18 : scriptdata,
                            },
                        default = Pass
                        )
                 )
                 

flv_header = Struct("FLV Header",
                    String("Signature", 3),
                    UBInt8("Version"),
                    UBInt8("TypeFlags"),
                    UBInt32("DataOffset"),
                    UBInt32("PreviousTagSize")
                    )

flv_tag_full = Struct("FLV Tag full",
                      Embed(flv_tag),
                      UBInt32("PreviousTagSize")
                      )
                    
flv = Struct("FLV",
             flv_header,
             MetaRepeater(1, flv_tag_full)
             )
