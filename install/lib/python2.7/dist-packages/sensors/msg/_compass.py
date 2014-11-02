"""autogenerated by genpy from sensors/compass.msg. Do not edit."""
import sys
python3 = True if sys.hexversion > 0x03000000 else False
import genpy
import struct


class compass(genpy.Message):
  _md5sum = "4b9d9eecbee8c0fcf7f1f2c957b3f53e"
  _type = "sensors/compass"
  _has_header = False #flag to mark the presence of a Header object
  _full_text = """int32 id
int32[10] rz
int32 head_init

"""
  __slots__ = ['id','rz','head_init']
  _slot_types = ['int32','int32[10]','int32']

  def __init__(self, *args, **kwds):
    """
    Constructor. Any message fields that are implicitly/explicitly
    set to None will be assigned a default value. The recommend
    use is keyword arguments as this is more robust to future message
    changes.  You cannot mix in-order arguments and keyword arguments.

    The available fields are:
       id,rz,head_init

    :param args: complete set of field values, in .msg order
    :param kwds: use keyword arguments corresponding to message field names
    to set specific fields.
    """
    if args or kwds:
      super(compass, self).__init__(*args, **kwds)
      #message fields cannot be None, assign default values for those that are
      if self.id is None:
        self.id = 0
      if self.rz is None:
        self.rz = [0,0,0,0,0,0,0,0,0,0]
      if self.head_init is None:
        self.head_init = 0
    else:
      self.id = 0
      self.rz = [0,0,0,0,0,0,0,0,0,0]
      self.head_init = 0

  def _get_types(self):
    """
    internal API method
    """
    return self._slot_types

  def serialize(self, buff):
    """
    serialize message into buffer
    :param buff: buffer, ``StringIO``
    """
    try:
      buff.write(_struct_i.pack(self.id))
      buff.write(_struct_10i.pack(*self.rz))
      buff.write(_struct_i.pack(self.head_init))
    except struct.error as se: self._check_types(struct.error("%s: '%s' when writing '%s'" % (type(se), str(se), str(_x))))
    except TypeError as te: self._check_types(ValueError("%s: '%s' when writing '%s'" % (type(te), str(te), str(_x))))

  def deserialize(self, str):
    """
    unpack serialized message in str into this message instance
    :param str: byte array of serialized message, ``str``
    """
    try:
      end = 0
      start = end
      end += 4
      (self.id,) = _struct_i.unpack(str[start:end])
      start = end
      end += 40
      self.rz = _struct_10i.unpack(str[start:end])
      start = end
      end += 4
      (self.head_init,) = _struct_i.unpack(str[start:end])
      return self
    except struct.error as e:
      raise genpy.DeserializationError(e) #most likely buffer underfill


  def serialize_numpy(self, buff, numpy):
    """
    serialize message with numpy array types into buffer
    :param buff: buffer, ``StringIO``
    :param numpy: numpy python module
    """
    try:
      buff.write(_struct_i.pack(self.id))
      buff.write(self.rz.tostring())
      buff.write(_struct_i.pack(self.head_init))
    except struct.error as se: self._check_types(struct.error("%s: '%s' when writing '%s'" % (type(se), str(se), str(_x))))
    except TypeError as te: self._check_types(ValueError("%s: '%s' when writing '%s'" % (type(te), str(te), str(_x))))

  def deserialize_numpy(self, str, numpy):
    """
    unpack serialized message in str into this message instance using numpy for array types
    :param str: byte array of serialized message, ``str``
    :param numpy: numpy python module
    """
    try:
      end = 0
      start = end
      end += 4
      (self.id,) = _struct_i.unpack(str[start:end])
      start = end
      end += 40
      self.rz = numpy.frombuffer(str[start:end], dtype=numpy.int32, count=10)
      start = end
      end += 4
      (self.head_init,) = _struct_i.unpack(str[start:end])
      return self
    except struct.error as e:
      raise genpy.DeserializationError(e) #most likely buffer underfill

_struct_I = genpy.struct_I
_struct_i = struct.Struct("<i")
_struct_10i = struct.Struct("<10i")
