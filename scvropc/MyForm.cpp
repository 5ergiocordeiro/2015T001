#include "MyForm.h"

using namespace System;
using namespace System::Windows::Forms;


void EndOPC(void);
int InitData(void);

#define MAX_PATH 1000
[STAThread]
int main(void) {
	// Verificar duplica��o {
		// cerr << "Duplicado!";
		// return 1;
		//}
	// Inicializa o ambiente
	Application::EnableVisualStyles;
	Application::SetCompatibleTextRenderingDefault(false);
	int retcode = InitData();
	if (retcode != 0) {
		return retcode;
		}
	scvropc::MyForm form;
	// Exibe a interface com o usu�rio
	form.enbupd();
	Application::Run(%form);

	EndOPC();
	return 0;
	}
