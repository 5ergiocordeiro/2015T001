#include "MyForm.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Threading;


void EndOPC(void);
int InitData(char ptags[MAX_ITEMS][50]);

#define MAX_PATH 1000
[STAThread]
int main(void) {
	// Verifica se a aplicação já está rodando
	Mutex^ mutex = gcnew Mutex(false, "SCVROPC");
	if (! mutex -> WaitOne(0, false)) {
		return 1;
		}
	// Inicializa o ambiente
	Application::EnableVisualStyles;
	Application::SetCompatibleTextRenderingDefault(false);
	char ptags[MAX_ITEMS][50];
	int retcode = InitData(ptags);
	if (retcode != 0) {
		return retcode;
		}
	scvropc::MyForm form;
	// Exibe a interface com o usuário
	form.enbupd(ptags);
	Application::Run(%form);

	EndOPC();
	mutex -> ReleaseMutex();
	return 0;
	}
