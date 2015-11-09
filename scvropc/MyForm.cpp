#include "MyForm.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Diagnostics;


int InitOPC(void);
void EndOPC(void);
void ReadAllItems(double * pval);
#define MAX_PATH 1000
[STAThread]
int main(int argc, char * argv[]) {
// Syntaxe:
//   scvropc {-s server_no} {-v verbose_level:0} {-zh} {-zd}
//   scvropc -s3 -t server_type -n tagname1,tagname2 ... tagname9 -m nick1,nick2 ... nick9 {-v verbose_level:0} {-zh} {-zd} {-b}
// server_no: 0 a 3, default 0
// verbose_level: 0 a 3, default 0
//	3: trace para obter informa��es necess�rias para o desenvolvimento
//	2: trace detalhado para acompanhamento da execu��o
//	1: trace resumido para acompanhamento da execu��o
//	0: apenas mensagens de erro s�o exibidas
// -zh, -zd N�o zerar os valores nas fronteiras de hora e dia
// -b remover pontos de milhar
// server_type: Excel ou Fix32
// tagname1 a tagname9: tags
// nick1 a nick9: nicknames
// Exemplos:
//   rdDDE
//     l� do servidor de produ��o com todas as configura��es default
//   rdDDE -s0
//     l� do servidor de produ��o com todas as configura��es default
//   rdDDE -s1
//     l� do servidor de testes 1 com todas as configura��es default
//   rdDDE -s3 -t Fix32 -n 
//     l� do servidor de testes 3, que � um Fix32, as tags 
	char szFileName[MAX_PATH];
	GetModuleBaseName(Process::GetCurrentProcess(), szFileName, MAX_PATH);
	Process::GetProcessesByName(szFileName);
	//if (Process::GetProcessesByName(aProcName).Length > 1)
		cerr << "Duplicado!";
		return 1;
		//}
	Application::EnableVisualStyles;
	Application::SetCompatibleTextRenderingDefault(false);
	int retcode = InitData(argc, argv);
	if (retcode != 0) {
		return retcode;
		}
	retcode = InitOPC();
	if (retcode != 0) {
		return retcode;
		}
	scvropc::MyForm form;
	form.enbupd();
	Application::Run(%form);
	EndOPC();
	return 0;
	}
