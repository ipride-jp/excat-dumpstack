1.agent���w�肷��ƁAAgentRoution.cpp�ɂ���Agent_OnLoad�֐������s�����B
2.Agent_OnLoad�ł́AVM�̊e��CallBack���w�肷��B�w�肳���֐��́A�N���XAgentCallbackHandler�ɒ�`����Ă���B
3.�ȉ��̎w��ŁASingal�ɂ��X���b�h�_���v���s���B
    callbacks.DataDumpRequest = &AgentCallbackHandler::dataDumpRequest;
  �@�@�� AgentRoutine::activateAgentThread()
        ��AgentRoutine::dumpAllThreadsForSignal()
         ��AgentRoutine::dumpStackOneThreadForSignal()
          ��stackTrace->writeToFile(fileNameBuf.c_str(),jni);
4.Excat�̊Ď��c�[���ɌĂ΂��Java�N���X�́AHelloWorld_forJava5�̃v���W�F�N�g�Œ�`����B

  Callbacks
    callback()
    native dumpstack()
    native dumpstackForMethod()
    native registerInstance()
    sendMailForException()
    sendMailForMethod()

  DumpMailer
    send()          