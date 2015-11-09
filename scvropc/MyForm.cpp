#include "MyForm.h"

using namespace System;
using namespace System::Windows::Forms;

int InitOPC(void);
void EndOPC(void);
void ReadAllItems(double * pval);

[STAThread]
int main(){
	Application::EnableVisualStyles;
	Application::SetCompatibleTextRenderingDefault(false);
	int retcode = InitOPC();
	if (retcode != 0) {
		return retcode;
		}
	double varValue[3];
	ReadAllItems(varValue);
	EndOPC();
	scvropc::MyForm form;
	form.MyRefresh(varValue[0], varValue[1], varValue[2]);
	Application::Run(%form);
	}


