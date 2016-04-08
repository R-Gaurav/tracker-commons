#include "octaveWconPythonWrapper.h"

#include <Python.h>

#include <iostream>
#include <stdlib.h> // for srand and rand
using namespace std;

// Included here because we need declarations from Python.h
#include "wrapperInternal.h"

PyObject *wrapperGlobalModule=NULL;
PyObject *wrapperGlobalWCONWormsClassObj=NULL;
PyObject *wrapperGlobalMeasurementUnitClassObj=NULL;

// Am exposing this as a wrapper interface method
//   because it is conceivable a user or some 
//   middleware tool might want to explicitly
//   invoke the initializer.
extern "C" int initOctaveWconPythonWrapper() {
  static bool isInitialized = false;

  PyObject *pErr;

  if (!isInitialized) {
    cout << "Initializing Embedded Python Interpreter" << endl;
    // initializing random number generator
    srand(1337);
    Py_Initialize();
    PyRun_SimpleString("import sys; sys.path.append('../../Python')\n");
    
    wrapperGlobalModule = PyImport_Import(PyUnicode_FromString("wcon"));
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      PyErr_Print();
      Py_XDECREF(wrapperGlobalModule);
      return 0; // failure condition
    }

    wrapperGlobalWCONWormsClassObj = 
      PyObject_GetAttrString(wrapperGlobalModule,"WCONWorms");
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      PyErr_Print();
      Py_XDECREF(wrapperGlobalModule);
      Py_XDECREF(wrapperGlobalWCONWormsClassObj);
      return 0; // failure condition
    }

    wrapperGlobalMeasurementUnitClassObj = 
      PyObject_GetAttrString(wrapperGlobalModule,"MeasurementUnit");
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      PyErr_Print();
      Py_XDECREF(wrapperGlobalModule);
      Py_XDECREF(wrapperGlobalWCONWormsClassObj);
      Py_XDECREF(wrapperGlobalMeasurementUnitClassObj);
      return 0; // failure condition
    }
    isInitialized = true;
  }

  return 1;
}

// *****************************************************************
// ********************** MeasurementUnit Class

// A Tentative C-only interface
//
// static_WCONWorms_load_from_file returns -1 for error
//    else a handle to be used by Octave/C/C++.
extern "C" int static_WCONWorms_load_from_file(const char *wconpath) {
  PyObject *pErr, *pFunc;
  int retCode;

  // TODO: Probably best done as a try-and-catch rather than
  //   via C error checking. Internal code can be pure C++.
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Attempting to load WCON file [" << wconpath << "]" << endl;

  pFunc = 
    PyObject_GetAttrString(wrapperGlobalWCONWormsClassObj,"load_from_file");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pFunc);
    return -1; // failure condition
  }

  if (PyCallable_Check(pFunc) == 1) {
    PyObject *pValue;
    pValue = PyObject_CallFunctionObjArgs(pFunc, 
					  PyUnicode_FromString(wconpath), 
					  NULL);
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      PyErr_Print();
      Py_XDECREF(pValue);
      Py_XDECREF(pFunc);
      return -1;
    }

    if (pValue != NULL) {
      // do not DECREF pValue until it is no longer referenced in the
      //   wrapper sublayer.
      int result = wrapInternalStoreReference(pValue);
      if (result == -1) {
	cout << "ERROR: failed to store object reference in wrapper." 
	     << endl;
	Py_DECREF(pValue);
      }
      Py_XDECREF(pFunc);
      return result;
    } else {
      cout << "ERROR: Null handle from load_from_file." << endl;
      // No need to DECREF a NULL pValue
      Py_XDECREF(pFunc);
      return -1;
    }
    Py_XDECREF(pFunc);
  } else {
    cout << "ERROR: load_from_file not a callable function" << endl;
    Py_XDECREF(pFunc);
    return -1;
  }
}

extern "C" int WCONWorms_save_to_file(const unsigned int selfHandle,
				      const char *output_path,
				      bool pretty_print,
				      bool compressed) {
  PyObject *WCONWorms_instance=NULL;
  PyObject *pErr, *pFunc;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Attempting to save WCON data to [" << output_path 
       << "] on object handle " << selfHandle << endl;

  WCONWorms_instance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_instance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  pFunc = 
    PyObject_GetAttrString(WCONWorms_instance,"save_to_file");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pFunc);
    return -1; // failure condition
  }

  if (PyCallable_Check(pFunc) == 1) {
    // NOTE: Am confused over why boolean literals in Python
    //   must respect reference count rules. Am tentatively
    //   not respecting them because I am afraid if I fail
    //   to DECREF it the correct number of times, things will
    //   blow up in my face. Something to keep an eye on.
    //
    // Keeping above notes, but answer can be found here:
    // http://stackoverflow.com/questions/28576775/python-c-api-boolean-objects

    Py_INCREF(Py_False);
    Py_INCREF(Py_False);
    PyObject *toPP = Py_False;
    PyObject *outputCompressed = Py_False;
    if (pretty_print) {
      Py_DECREF(Py_False);
      Py_INCREF(Py_True);
      toPP = Py_True;
    }
    if (compressed) {
      Py_DECREF(Py_False);
      Py_INCREF(Py_True);
      outputCompressed = Py_True;
    }
    PyObject_CallFunctionObjArgs(pFunc, 
				 PyUnicode_FromString(output_path),
				 toPP,
				 outputCompressed,
				 NULL);
    Py_DECREF(toPP);
    Py_DECREF(outputCompressed);
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      Py_DECREF(pFunc);
      PyErr_Print();
      return -1;
    } else {
      Py_DECREF(pFunc);
    }
  } else {
    cout << "ERROR: save_to_file not a callable function" << endl;
    Py_XDECREF(pFunc);
    return -1;
  }

  return 0;
}

extern "C" int WCONWorms_to_canon(const unsigned int selfHandle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Create canonical copy of instance " << selfHandle << endl;

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  // to_canon is implemented as an object property and not a function
  pAttr = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"to_canon");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return -1; // failure condition
  }

  if (pAttr != NULL) {
    // do not DECREF pAttr until it is no longer referenced in the
    //   wrapper sublayer.
    int result = wrapInternalStoreReference(pAttr);
    if (result == -1) {
	cout << "ERROR: failed to store object reference in wrapper." 
	     << endl;
	Py_DECREF(pAttr);
    }
    return result;
  } else {
    cout << "ERROR: Null handle from to_canon" << endl;
    // No need to DECREF a NULL pAttr
    return -1;
  }
}


// NOTE: The add operator had been implemented in Python as instance
//   methods with self-modification (merge) side effects. It is thus
//   not possible to implement something like x = y + z;
//
//  Updated: Apparently I'm wrong. The example tests (in Python) as
//   well as my own tests say that it returns a new object. As such
//   the default behavior is indeed x = y + z with no change to y.

//       We could however cheat for non-modifying operators like eq
//   if it turns out for there to be a use-case where it is better
//   to execute a class method instead.

// TODO: The following wrapper APIs really suggest the wrapper
//   interface is better off in C++ with operator overloading.
int WCONWorms_add(const unsigned int selfHandle, const unsigned int handle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *WCONWorms_instance=NULL;
  PyObject *pErr, *pFunc;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Add instance " << handle << " into instance "
       << selfHandle << endl;  

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: No self object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  WCONWorms_instance = wrapInternalGetReference(handle);
  if (WCONWorms_instance == NULL) {
    cerr << "ERROR: No self object instance using handle "
	 << handle << endl;
    return -1;
  }

  pFunc = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"__add__");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pFunc);
    return -1; // failure condition
  }

  if (PyCallable_Check(pFunc) == 1) {
    PyObject *pValue;
    pValue = PyObject_CallFunctionObjArgs(pFunc, 
					  WCONWorms_instance,
					  NULL);
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      Py_DECREF(pFunc);
      PyErr_Print();
      return -1;
    } else {
      if (pValue != NULL) {
	// Do not DECREF stored pValue
	int result = wrapInternalStoreReference(pValue);
	if (result == -1) {
	  cout << "ERROR: failed to store object reference in wrapper." 
	       << endl;
	  Py_DECREF(pValue);
	}
	Py_XDECREF(pFunc);
	return result;
      } else {
	cerr << "ERROR: add failed to produce a valid resulting object"
	     << endl;
	// no need to DECREF a NULL pValue
	Py_DECREF(pFunc);
	return -1;
      }
    }
  } else {
    cout << "ERROR: save_to_file not a callable function" << endl;
    Py_XDECREF(pFunc);
    return -1;
  }
  return 1; // success
}

// TODO: The tentative need to return an int for error conditions
//   instead of a bool is annoying, but can be addressed by having
//   the error condition be a parameter like in the case of the
//   MPI API implementation.
int WCONWorms_eq(const unsigned int selfHandle, const unsigned int handle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *WCONWorms_instance=NULL;
  PyObject *pErr, *pFunc;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Is instance " << selfHandle << " equal to instance "
       << handle << "?" << endl;

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: No self object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  WCONWorms_instance = wrapInternalGetReference(handle);
  if (WCONWorms_instance == NULL) {
    cerr << "ERROR: No self object instance using handle "
	 << handle << endl;
    return -1;
  }

  pFunc = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"__eq__");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pFunc);
    return -1; // failure condition
  }

  if (PyCallable_Check(pFunc) == 1) {
    // NOTE: This may explain why Py_True and Py_False has reference
    //   counts. Because PyObject * is anonymous, and because the
    //   system automatically INCREFs return values. Users are then
    //   expected to DECREF objects acquired from embedding as a 
    //   default. See my notes on this in save_to_file. The way
    //   I see it, if I don't acquire those 2 literals via an interface
    //   call, I should not need to DECREF it. Keep an eye on this.
    //   
    //   Python reference count problems tend to manifest themselves as
    //   a segfault.
    PyObject *retValue;
    retValue = PyObject_CallFunctionObjArgs(pFunc, 
					    WCONWorms_instance,
					    NULL);
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      Py_XDECREF(retValue);
      Py_DECREF(pFunc);
      PyErr_Print();
      return -1;
    } else {
      if (PyObject_IsTrue(retValue)) {
	Py_DECREF(retValue);
	return 1; // true
      } else {
	Py_DECREF(retValue);
	return 0; // false
      }
      Py_DECREF(pFunc);
    }
  } else {
    cout << "ERROR: save_to_file not a callable function" << endl;
    Py_XDECREF(pFunc);
    return -1;
  }
}

extern "C" int WCONWorms_units(const unsigned int selfHandle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Retrieve units property of handle " << selfHandle << endl;

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  // Attribute is a Python dict (Dictionary) object.
  //   We are not going to mess with its internal structure for now.
  //   Experience informs me that it is best to build an auxilary
  //   interface for Octave to issue Dictionary-modifying commands
  //   to the Python runtime, than to attempt to make its own copy
  //   or have a copy managed by C/C++ (loosely related to threading
  //   consistency issues.)
  pAttr = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"units");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return -1;
  }

  if (pAttr != NULL) {
    if (PyDict_Check(pAttr)) {
      // TODO: Consider if we should maintain a separate hash table
      //   for different object types. I'm thinking no, but I'm not sure.
      int result = wrapInternalStoreReference(pAttr);
      if (result == -1) {
	cout << "ERROR: failed to store object reference in wrapper." 
	     << endl;
	Py_DECREF(pAttr);
      }
      return result;
    } else {
      cout << "ERROR: units should be a dict object but is not!" << endl;
      Py_DECREF(pAttr);
      return -1;
    }
  } else {
    cout << "ERROR: Null handle from units" << endl;
    // No need to DECREF a NULL pAttr
    return -1;
  }
}

extern "C" int WCONWorms_metadata(const unsigned int selfHandle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Retrieve metadata property of handle " << selfHandle << endl;

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  // Attribute is a Python dict (Dictionary) object with complex members
  pAttr = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"metadata");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return -1;
  }

  if (pAttr != NULL) {
    // checking for None at the top makes it possible to
    // cascade the conditionals because of the need to
    // handle reference counts.
    Py_INCREF(Py_None);
    // C/C++ equality is supposed to work
    if (pAttr == Py_None) {
      Py_DECREF(Py_None);
      return -1337;
    } else if (PyDict_Check(pAttr)) {
      // TODO: Consider if we should maintain a separate hash table
      //   for different object types. I'm thinking no, but I'm not sure.
      int result = wrapInternalStoreReference(pAttr);
      if (result == -1) {
	cout << "ERROR: failed to store object reference in wrapper." 
	     << endl;
	Py_DECREF(pAttr);
      }
      Py_DECREF(Py_None);
      return result;
    } else {
      cout << "ERROR: metadata should be a dict object " 
	   << "or None, but is neither!" << endl;
      Py_DECREF(Py_None);
      Py_DECREF(pAttr);
      return -1;
    }
  } else {
    cout << "ERROR: metadata is NULL" << endl;
    // No need to DECREF a NULL pAttr
    return -1;
  }
}

extern "C" int WCONWorms_data(const unsigned int selfHandle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Retrieve data property of handle " << selfHandle << endl;

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  // Attribute is a pandas package DataFrame object.
  //   I currently have no clue what that is, so I'm leaving out
  //   any error checks until I figure it out.
  pAttr = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"data");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return -1;
  }

  if (pAttr != NULL) {
    int result = wrapInternalStoreReference(pAttr);
    if (result == -1) {
      cout << "ERROR: failed to store object reference in wrapper." 
	   << endl;
      Py_DECREF(pAttr);
    }
    return result;
  } else {
    cout << "ERROR: Null handle from data" << endl;
    // No need to DECREF a NULL pAttr
    return -1;
  }
}

extern "C" long WCONWorms_num_worms(const unsigned int selfHandle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1; // still ok as an error condition. num_worms >= 0.
  }
  cout << "Retrieve num_worms property of handle " << selfHandle << endl;

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  pAttr = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"num_worms");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return -1;
  }

  if (pAttr != NULL) {
    long result;
    result = PyLong_AsLong(pAttr);
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      PyErr_Print();
      Py_DECREF(pAttr);
      cerr << "ERROR: Could not convert num_worms to a long value" << endl;
      return -1;
    }
    return result;
  } else {
    cout << "ERROR: Null handle from num_worms" << endl;
    // No need to DECREF a NULL pAttr
    return -1;
  }
}

extern "C" int WCONWorms_worm_ids(const unsigned int selfHandle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Retrieve metadata property of handle " << selfHandle << endl;

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  // Attribute is a Python list object (of worm ids - of some type)
  //   I've seen integers as well as strings, so that needs to be
  //   sorted out as well.
  pAttr = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"worm_ids");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return -1;
  }

  if (pAttr != NULL) {
    if (PyList_Check(pAttr)) {
      // TODO: Consider if we should maintain a separate hash table
      //   for different object types. I'm thinking no, but I'm not sure.
      int result = wrapInternalStoreReference(pAttr);
      if (result == -1) {
	cout << "ERROR: failed to store object reference in wrapper." 
	     << endl;
	Py_DECREF(pAttr);
      }
      return result;
    } else {
      cout << "ERROR: worm_ids should be a list object but is not!" << endl;
      Py_DECREF(pAttr);
      return -1;
    }
  } else {
    cout << "ERROR: Null handle from worm_ids" << endl;
    // No need to DECREF a NULL pAttr
    return -1;
  }
}

extern "C" int WCONWorms_data_as_odict(const unsigned int selfHandle) {
  PyObject *WCONWorms_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Retrieve metadata property of handle " << selfHandle << endl;

  WCONWorms_selfInstance = wrapInternalGetReference(selfHandle);
  if (WCONWorms_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  // Attribute is an OrderedDict object which is a subclass of Python's
  //   dict object implemented in the collections package. There should
  //   be no harm checking and treating the object as a regular dict
  //   object.
  pAttr = 
    PyObject_GetAttrString(WCONWorms_selfInstance,"data_as_odict");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return -1;
  }

  if (pAttr != NULL) {
    if (PyDict_Check(pAttr)) {
      // TODO: Consider if we should maintain a separate hash table
      //   for different object types. I'm thinking no, but I'm not sure.
      int result = wrapInternalStoreReference(pAttr);
      if (result == -1) {
	cout << "ERROR: failed to store object reference in wrapper." 
	     << endl;
	Py_DECREF(pAttr);
      }
      return result;
    } else {
      cout << "ERROR: data_as_odict should be a dict object but is not!" 
	   << endl;
      Py_DECREF(pAttr);
      return -1;
    }
  } else {
    cout << "ERROR: Null handle from data_as_odict" << endl;
    // No need to DECREF a NULL pAttr
    return -1;
  }
}


// *****************************************************************
// ********************** MeasurementUnit Class
extern "C" int static_MeasurementUnit_create(const char *unitStr) {
  PyObject *pErr, *pFunc;
  int retCode;

  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1;
  }
  cout << "Attempting to create a MeasurementUnit object with ["
       << unitStr << "]" << endl;

  pFunc = 
    PyObject_GetAttrString(wrapperGlobalMeasurementUnitClassObj,
			   "create");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pFunc);
    return -1; // failure condition
  }

  if (PyCallable_Check(pFunc) == 1) {
    PyObject *pValue;
    pValue = PyObject_CallFunctionObjArgs(pFunc, 
					  PyUnicode_FromString(unitStr), 
					  NULL);
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      PyErr_Print();
      Py_XDECREF(pValue);
      Py_XDECREF(pFunc);
      return -1;
    }

    if (pValue != NULL) {
      // do not DECREF pValue until it is no longer referenced in the
      //   wrapper sublayer.
      int result = wrapInternalStoreReference(pValue);
      if (result == -1) {
	cout << "ERROR: failed to store object reference in wrapper." 
	     << endl;
	Py_DECREF(pValue);
      }
      Py_XDECREF(pFunc);
      return result;
    } else {
      cout << "ERROR: Null handle from create" << endl;
      // No need to DECREF a NULL pValue
      Py_XDECREF(pFunc);
      return -1;
    }
    Py_XDECREF(pFunc);
  } else {
    cout << "ERROR: create not a callable function" << endl;
    Py_XDECREF(pFunc);
    return -1;
  }
}

// TODO: Ok ... for these I'm not even going to try dealing with error
//   codes
extern "C" double MeasurementUnit_to_canon(const unsigned int selfHandle,
					   const double val) {
  PyObject *MeasurementUnit_instance=NULL;
  PyObject *pErr, *pFunc;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1; // closest approximation to a failure condition for now
  }
  cout << "Converting native value to canonical for MeasurementUnit "
       << selfHandle << endl;

  MeasurementUnit_instance = wrapInternalGetReference(selfHandle);
  if (MeasurementUnit_instance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  pFunc = 
    PyObject_GetAttrString(MeasurementUnit_instance,"to_canon");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pFunc);
    return -1; 
  }

  if (PyCallable_Check(pFunc) == 1) {
    PyObject *retValue;
    retValue = PyObject_CallFunctionObjArgs(pFunc, 
					    PyFloat_FromDouble(val),
					    NULL);
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      Py_XDECREF(retValue);
      Py_DECREF(pFunc);
      PyErr_Print();
      return -1;
    } else {
      if (retValue != NULL) {
	double retCValue;
	retCValue = PyFloat_AsDouble(retValue);
	pErr = PyErr_Occurred();
	Py_DECREF(retValue);
	Py_DECREF(pFunc);
	if (pErr != NULL) {
	  PyErr_Print();
	  return -1;
	} else {
	  return retCValue;
	}
      } else {
	// Nothing to DECREF for NULL retValue
	Py_DECREF(pFunc);
	return -1;
      }
    }
  } else {
    cout << "ERROR: to_canon not a callable function" << endl;
    Py_XDECREF(pFunc);
    return -1;
  }
}

extern "C" double MeasurementUnit_from_canon(const unsigned int selfHandle,
					     const double val) {
  PyObject *MeasurementUnit_instance=NULL;
  PyObject *pErr, *pFunc;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return -1; // closest approximation to a failure condition for now
  }
  cout << "Converting native value to canonical for MeasurementUnit "
       << selfHandle << endl;

  MeasurementUnit_instance = wrapInternalGetReference(selfHandle);
  if (MeasurementUnit_instance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return -1;
  }

  pFunc = 
    PyObject_GetAttrString(MeasurementUnit_instance,"from_canon");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pFunc);
    return -1; 
  }

  if (PyCallable_Check(pFunc) == 1) {
    PyObject *retValue;
    retValue = PyObject_CallFunctionObjArgs(pFunc, 
					    PyFloat_FromDouble(val),
					    NULL);
    pErr = PyErr_Occurred();
    if (pErr != NULL) {
      Py_XDECREF(retValue);
      Py_DECREF(pFunc);
      PyErr_Print();
      return -1;
    } else {
      if (retValue != NULL) {
	double retCValue;
	retCValue = PyFloat_AsDouble(retValue);
	pErr = PyErr_Occurred();
	Py_DECREF(retValue);
	Py_DECREF(pFunc);
	if (pErr != NULL) {
	  PyErr_Print();
	  return -1;
	} else {
	  return retCValue;
	}
      } else {
	// Nothing to DECREF for NULL retValue
	Py_DECREF(pFunc);
	return -1;
      }
    }
  } else {
    cout << "ERROR: from_canon not a callable function" << endl;
    Py_XDECREF(pFunc);
    return -1;
  }
}

// TODO: These two API calls just seal the deal on whether to adopt
//       MPI-style error code return mechanisms. Not that it was in
//       any real doubt.

extern "C" 
const char *MeasurementUnit_unit_string(const unsigned int selfHandle) {
  PyObject *MeasurementUnit_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return NULL;
  }
  cout << "Retrieve unit_string property of handle " << selfHandle << endl;

  MeasurementUnit_selfInstance = wrapInternalGetReference(selfHandle);
  if (MeasurementUnit_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return NULL;
  }

  pAttr = 
    PyObject_GetAttrString(MeasurementUnit_selfInstance,"unit_string");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return NULL;
  }

  if (pAttr != NULL) {
    // TODO: This is quite a pain in the butt that will have to
    //   be looked into further for robustness issues. It sure looks
    //   like we're hitting some Python 2 versus Python 3 issues as well.
    char *result;
    result = PyBytes_AsString(PyUnicode_AsASCIIString(pAttr));
    return result;
  } else {
    cout << "ERROR: Null handle from unit_string" << endl;
    // No need to DECREF a NULL pAttr
    return NULL;
  }
}

extern "C" const char *MeasurementUnit_canonical_unit_string(const unsigned int selfHandle) {
  PyObject *MeasurementUnit_selfInstance=NULL;
  PyObject *pErr, *pAttr;
  int retCode;
  
  retCode = initOctaveWconPythonWrapper();
  if (retCode == 0) {
    cout << "Failed to initialize wrapper library." << endl;
    return NULL;
  }
  cout << "Retrieve canonical_unit_string property of handle " << selfHandle << endl;

  MeasurementUnit_selfInstance = wrapInternalGetReference(selfHandle);
  if (MeasurementUnit_selfInstance == NULL) {
    cerr << "ERROR: Failed to acquire object instance using handle "
	 << selfHandle << endl;
    return NULL;
  }

  pAttr = 
    PyObject_GetAttrString(MeasurementUnit_selfInstance,
			   "canonical_unit_string");
  pErr = PyErr_Occurred();
  if (pErr != NULL) {
    PyErr_Print();
    Py_XDECREF(pAttr);
    return NULL;
  }

  if (pAttr != NULL) {
    // TODO: This is quite a pain in the butt that will have to
    //   be looked into further for robustness issues. It sure looks
    //   like we're hitting some Python 2 versus Python 3 issues as well.
    char *result;
    result = PyBytes_AsString(PyUnicode_AsASCIIString(pAttr));
    return result;
  } else {
    cout << "ERROR: Null handle from canonical_unit_string" << endl;
    // No need to DECREF a NULL pAttr
    return NULL;
  }
}
