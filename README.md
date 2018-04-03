1. agentを指定すると、AgentRoution.cppにあるAgent_OnLoad関数が実行される。
1. Agent_OnLoadでは、VMの各種CallBackを指定する。指定される関数は、クラスAgentCallbackHandlerに定義されている。
1. 以下の指定で、Singalによるスレッドダンプを行う。
  ```
      callbacks.DataDumpRequest = &AgentCallbackHandler::dataDumpRequest;
    　　→ AgentRoutine::activateAgentThread()
          →AgentRoutine::dumpAllThreadsForSignal()
           →AgentRoutine::dumpStackOneThreadForSignal()
            →stackTrace->writeToFile(fileNameBuf.c_str(),jni);
  ```
1. Excatの監視ツールに呼ばれるJavaクラスは、HelloWorld_forJava5のプロジェクトで定義する。
  ```
    Callbacks
      callback()
      native dumpstack()
      native dumpstackForMethod()
      native registerInstance()
      sendMailForException()
      sendMailForMethod()

    DumpMailer
      send()
  ```
