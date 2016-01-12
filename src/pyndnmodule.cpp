#include <Python.h>
#include <ndn-cpp/lite/encoding/tlv-0_1_1-wire-format-lite.hpp>

using namespace ndn;

extern "C" {
  static PyObject *
  _pyndn_Tlv0_1_1WireFormat_encodeData(PyObject *self, PyObject *args);
}

/**
 * When PyObjectRef goes out of scope, it calls Py_DECREF on the PyObject given
 * to the constructor.
 */
class PyObjectRef {
public:
  PyObjectRef(PyObject* obj) : obj(obj) {}
  ~PyObjectRef() { if (obj) Py_DECREF(obj); }
  operator PyObject*() { return obj; }
  PyObject* obj;
};

static long
toLongByMethod(PyObject* obj, const char* methodName)
{
  PyObjectRef val(PyObject_CallMethod(obj, (char*)methodName, (char*)""));
  return PyInt_AsLong(val);
}

static double
toDoubleByMethod(PyObject* obj, const char* methodName)
{
  PyObjectRef val(PyObject_CallMethod(obj, (char*)methodName, (char*)""));
  return PyFloat_AsDouble(val);
}

/**
 * Get a Blob by calling obj.methodName() and imitate Blob.toBuffer to get
 * the array in a BlobLite.
 * @param obj The object with the Blob.
 * @param methodName The method name that returns a Blob.
 * @return A BlobLite which points into the raw bytes, or an empty BlobLite if
 * if the Blob isNull.
 */
static BlobLite
toBlobLiteByMethod(PyObject* obj, const char* methodName)
{
  PyObjectRef blob(PyObject_CallMethod(obj, (char*)methodName, (char*)""));

  // Imitate Blob.toBuffer to be bufferObj.
  PyObjectRef blobArray(PyObject_GetAttrString(blob, "_array"));
  if (blobArray.obj == Py_None)
    return BlobLite();

  PyObject* bufferObj;
  PyObjectRef blobArrayView(0);
  if (PyObject_HasAttrString(blobArray, "_view")) {
    // Assume _array is a _memoryviewWrapper.
    blobArrayView.obj = PyObject_GetAttrString(blobArray, "_view");
    bufferObj = blobArrayView.obj;
  }
  else
    bufferObj = blobArray.obj;

  Py_buffer bufferView;
  if (PyObject_GetBuffer(bufferObj, &bufferView, PyBUF_SIMPLE) != 0)
    // Error. Don't expect this to happen, so just return an empty blob.
    return BlobLite();
  // TODO: Check if it is a byte buffer?
  uint8_t* buf = (uint8_t*)bufferView.buf;
  size_t size = bufferView.len;
  // TODO: Do we have to put this in a pool to be freed later?
  PyBuffer_Release(&bufferView);

  return BlobLite(buf, size);
}

// Imitate Name::get(NameLite& nameLite).
static void
toNameLite(PyObject* name, NameLite& nameLite)
{
  nameLite.clear();
  PyObjectRef components(PyObject_GetAttrString(name, "_components"));
  for (size_t i = 0; i < PyList_GET_SIZE(components.obj); ++i) {
    ndn_Error error;
    if ((error = nameLite.append
         (toBlobLiteByMethod
          (PyList_GET_ITEM(components.obj, i), "getValue"))))
      // TODO: Handle the error!
      return;
  }
}

// Imitate KeyLocator::get(KeyLocatorLite& keyLocatorLite).
static void
toKeyLocatorLite(PyObject* keyLocator, KeyLocatorLite& keyLocatorLite)
{
  keyLocatorLite.setType
    ((ndn_KeyLocatorType)(int)toLongByMethod(keyLocator, "getType"));
  keyLocatorLite.setKeyData
    (toBlobLiteByMethod(keyLocator, "getKeyData"));

  PyObjectRef keyName(PyObject_CallMethod(keyLocator, (char*)"getKeyName", (char*)""));
  toNameLite(keyName, keyLocatorLite.getKeyName());
}

// Imitate Sha256WithRsaSignature::get(SignatureLite& signatureLite).
static void
toSha256WithRsaSignatureLite(PyObject* signature, SignatureLite& signatureLite)
{
  signatureLite.setType(ndn_SignatureType_Sha256WithRsaSignature);
  signatureLite.setSignature(toBlobLiteByMethod(signature, "getSignature"));

  PyObjectRef keyLocator
    (PyObject_CallMethod(signature, (char*)"getKeyLocator", (char*)""));
  toKeyLocatorLite(keyLocator, signatureLite.getKeyLocator());
}

// Imitate MetaInfo::get(MetaInfoLite& metaInfoLite).
static void
toMetaInfoLite(PyObject* metaInfo, MetaInfoLite& metaInfoLite)
{
  metaInfoLite.setType((ndn_ContentType)(int)toLongByMethod(metaInfo, "getType"));
  metaInfoLite.setFreshnessPeriod(toDoubleByMethod(metaInfo, "getFreshnessPeriod"));
  PyObjectRef finalBlockId(PyObject_CallMethod
    (metaInfo, (char*)"getFinalBlockId", (char*)""));
  metaInfoLite.setFinalBlockId(NameLite::Component
    (toBlobLiteByMethod(finalBlockId, "getValue")));
}

// Imitate Data::get(DataLite& dataLite).
static void
toDataLite(PyObject* data, DataLite& dataLite)
{
  // TODO: Handle types other than Sha256WithRsaSignature
  PyObjectRef signature(PyObject_CallMethod(data, (char*)"getSignature", (char*)""));
  toSha256WithRsaSignatureLite(signature, dataLite.getSignature());

  PyObjectRef name(PyObject_CallMethod(data, (char*)"getName", (char*)""));
  toNameLite(name, dataLite.getName());

  PyObjectRef metaInfo(PyObject_CallMethod(data, (char*)"getMetaInfo", (char*)""));
  toMetaInfoLite(metaInfo, dataLite.getMetaInfo());

  dataLite.setContent(toBlobLiteByMethod(data, "getContent"));
}

static PyObject *
_pyndn_Tlv0_1_1WireFormat_encodeData(PyObject *self, PyObject *args)
{
  PyObject* data;
  if (!PyArg_ParseTuple(args, "O", &data))
    return NULL;

  struct ndn_NameComponent nameComponents[100];
  struct ndn_NameComponent keyNameComponents[100];
  DataLite dataLite
    (nameComponents, sizeof(nameComponents) / sizeof(nameComponents[0]),
     keyNameComponents, sizeof(keyNameComponents) / sizeof(keyNameComponents[0]));

  toDataLite(data, dataLite);

  // TODO: Make this a dynamic buffer.
  uint8_t debugEncoding[2000];
  DynamicUInt8ArrayLite output(debugEncoding, sizeof(debugEncoding), 0);
  size_t signedPortionBeginOffset, signedPortionEndOffset, encodingLength;

  ndn_Error error;
  if ((error = Tlv0_1_1WireFormatLite::encodeData
       (dataLite, &signedPortionBeginOffset, &signedPortionEndOffset,
        output, &encodingLength))) {
    PyErr_SetString(PyExc_RuntimeError, ndn_getErrorString(error));
    return 0;
  }

  // TODO: Does returning a str work in Python 3 (where this is Unicode)?
  return Py_BuildValue
    ("(s#,i,i)", debugEncoding, encodingLength, (int)signedPortionBeginOffset,
     (int)signedPortionEndOffset);
}

extern "C" {
  static PyMethodDef PyndnMethods[] = {
    {"Tlv0_1_1WireFormat_encodeData",  _pyndn_Tlv0_1_1WireFormat_encodeData, METH_VARARGS,
"Encode data in NDN-TLV and return the encoding and signed offsets.\n\
\n\
:param Data data: The Data object to encode.\n\
:return: A Tuple of (encoding, signedPortionBeginOffset,\n\
  signedPortionEndOffset) where encoding is a raw str (not Blob) containing the\n\
  encoding, signedPortionBeginOffset is the offset in the encoding of\n\
  the beginning of the signed portion, and signedPortionEndOffset is\n\
  the offset in the encoding of the end of the signed portion. If r is the\n\
  result Tuple, the encoding Blob is Blob(r[0], False).\n\
:rtype: (str, int, int)"},
    {NULL, NULL, 0, NULL} // sentinel
  };

  PyMODINIT_FUNC
  init_pyndn(void)
  {
    (void)Py_InitModule("_pyndn", PyndnMethods);
  }
}
