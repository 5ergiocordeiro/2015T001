#include "MyForm.h"

using namespace System;
using namespace System::Windows::Forms;


void EndOPC(void);
int InitData(char ptags[MAX_ITEMS][50]);

#define MAX_PATH 1000
[STAThread]
int main(void) {
	char ptags[MAX_ITEMS][50];
	// Verificar duplicação {
		// cerr << "Duplicado!";
		// return 1;
		//}
	// Inicializa o ambiente
	Application::EnableVisualStyles;
	Application::SetCompatibleTextRenderingDefault(false);
	int retcode = InitData(ptags);
	if (retcode != 0) {
		return retcode;
		}
	scvropc::MyForm form;
	// Exibe a interface com o usuário
	form.enbupd(ptags);
	Application::Run(%form);

	EndOPC();
	return 0;
	}
