#include <jni.h>
#include <string>
#include <thread>
#include <android/log.h>

#define _MAKE_JNI_FUNCTION(r, n, p) extern "C" JNIEXPORT r JNICALL Java_ ## p ## _ ## n
#define JNI(r, n) _MAKE_JNI_FUNCTION(r, n, com_stdlib_industrialice_stdlib_MainActivity)

extern int main(int argc, const char **argv);

namespace
{
    jclass JavaCallbackReceiverClass{};
    jobject JavaCallbackReceiverObject{};
    JavaVM *JVM;
    std::thread AppThread;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JVM = vm;

    JNIEnv *env;
    jint res = JVM->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (res != JNI_OK)
    {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

static JNIEnv *AcquireJNIEnvironment()
{
    JNIEnv *env;
    jint res = JVM->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (res != JNI_OK)
    {
        if (res == JNI_EDETACHED)
        {
            res = JVM->AttachCurrentThread(&env, NULL);
        }
        if (res != JNI_OK)
        {
            return nullptr;
        }
    }

    return env;
}

JNI(void, callIntoJNI)(JNIEnv *env, jobject callbackReceiver, jstring appPath)
{
    JavaCallbackReceiverObject = env->NewGlobalRef(callbackReceiver);
    JavaCallbackReceiverClass = (jclass)env->NewGlobalRef(env->GetObjectClass(callbackReceiver));

    const char *cStr = env->GetStringUTFChars(appPath, NULL);
    std::string cppStr = cStr;
    env->ReleaseStringUTFChars(appPath, cStr);

    auto runInThread = [](std::string appPath)
    {
        const char *args[] =
                {
                        "AndroidApp",
                        appPath.c_str()
                };
        main(sizeof(args) / sizeof(void *), args);
        JVM->DetachCurrentThread();
    };

    AppThread = std::thread(runInThread, cppStr);
}

void OnLogMessage(const char *message)
{
    __android_log_print(ANDROID_LOG_INFO, "StdLib", "%s", message);

    JNIEnv *env = AcquireJNIEnvironment();

    auto javaStr = env->NewStringUTF(message);
    jmethodID method = env->GetMethodID(JavaCallbackReceiverClass, "OnLogMessage", "(Ljava/lang/String;)V");
    if (!method)
    {
        __android_log_print(ANDROID_LOG_ERROR, "StdLib", "OnLogMessage failed to find Java method OnLogMessage to call\n");
        return;
    }

    env->CallVoidMethod(JavaCallbackReceiverObject, method, javaStr);
    env->DeleteLocalRef(javaStr);
}

void PopLastMessage(unsigned int length)
{
    JNIEnv *env = AcquireJNIEnvironment();

    jmethodID method = env->GetMethodID(JavaCallbackReceiverClass, "PopLastMessage", "(I)V");
    if (!method)
    {
        __android_log_print(ANDROID_LOG_ERROR, "StdLib", "PopLastMessage failed to find Java method PopLastMessage to call\n");
        return;
    }

    env->CallVoidMethod(JavaCallbackReceiverObject, method, length);
}

void DetachCurrentThread()
{
    JVM->DetachCurrentThread();
}