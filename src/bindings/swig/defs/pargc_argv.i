/* Modification of the argc_argv.i file from SWIG to deal with the fact that SimGrid uses an "int*" for argc
 * Changes compared to the original file:
 *  - Type of i and len is "int", not a SWIG macro to retrive the type of 1
 *  - Also map std::vector<std::string> and String[], both from C++ to Java and from Java to C++ 
 *
 * TODO: cache (or avoid alltogether) the call to env->FindClass("java/lang/String");
 * ------------------------------------------------------------- */

%typemap(jni) (int* argc, char** argv) "jobjectArray"
%typemap(jtype) (int* argc, char** argv) "String[]"
%typemap(jstype) (int* argc, char** argv) "String[]"

%typemap(in) (int* argc, char** argv) {
  if ($input == (jobjectArray)0) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return $null;
  }
  int len = (int)JCALL1(GetArrayLength, jenv, $input);
  if (len < 0) {
    SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, "array length negative");
    return $null;
  }
  $2 = ($2_ltype) malloc((len+1)*sizeof($*2_ltype));
  if ($2 == NULL) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "memory allocation failed");
    return $null;
  }
  $1 = &len;
  jsize i;
  for (i = 0; i < len; i++) {
    jstring j_string = (jstring)JCALL2(GetObjectArrayElement, jenv, $input, i);
    const char *c_string = JCALL2(GetStringUTFChars, jenv, j_string, 0);
    $2[i] = ($*2_ltype)c_string;
  }
  $2[i] = NULL;
}

%typemap(javain) (int* argc, char** argv) "$javainput"
%typemap(freearg) (int* argc, char** argv) {
  free((void *)$2);
}


///////////// 
%include "std_vector.i"
%include "std_string.i"
// Convert Java String[] into C++ std::vector<std::string>
%typemap(jni) std::vector<std::string> "jobjectArray"
%typemap(jtype) std::vector<std::string> "String[]"
%typemap(jstype) std::vector<std::string> "String[]"
%typemap(javain) std::vector<std::string> "$javainput"

%typemap(in) std::vector<std::string> (jobjectArray obj) {
    int len = (int)JCALL1(GetArrayLength, jenv, $input);
    $1.reserve(len);
    for (jsize i = 0; i < len; ++i) {
        jstring j_string = (jstring)JCALL2(GetObjectArrayElement, jenv, $input, i);
        const char *c_string = JCALL2(GetStringUTFChars, jenv, j_string, 0);
        $1.push_back(c_string);
        JCALL2(ReleaseStringUTFChars, jenv, j_string, c_string);
        JCALL1(DeleteLocalRef, jenv, j_string);
    }
}

// Convert C++ std::vector<std::string> into Java String[]
%typemap(out) std::vector<std::string> {
    JNIEnv *env = jenv;
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray obj = env->NewObjectArray($1.size(), stringClass, NULL);
    for (size_t i = 0; i < $1.size(); ++i) {
        env->SetObjectArrayElement(obj, i, env->NewStringUTF($1[i].c_str()));
    }
    $result = obj;
}
