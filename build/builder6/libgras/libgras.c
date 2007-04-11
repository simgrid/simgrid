//---------------------------------------------------------------------------

#include <windows.h>
//---------------------------------------------------------------------------
//   Remarque importante concernant la gestion de mémoire de DLL lorsque votre DLL utilise la
//   version statique de la bibliothèque d'exécution :
//
//   Si votre DLL exporte des fonctions qui passent des objets String (ou des
//   structures/classes contenant des chaînes imbriquées) comme paramètre
//   ou résultat de fonction, vous devrez ajouter la bibliothèque MEMMGR.LIB
//   à la fois au projet DLL et à tout projet qui utilise la DLL.  Vous devez aussi
//   utiliser MEMMGR.LIB si un projet qui utilise la DLL effectue des opérations
//   new ou delete sur n'importe quelle classe non dérivée de TObject qui est
//   exportée depuis la DLL. Ajouter MEMMGR.LIB à votre projet forcera la DLL et
//   ses EXE appelants à utiliser BORLNDMM.DLL comme gestionnaire de mémoire.
//   Dans ce cas, le fichier BORLNDMM.DLL devra être déployé avec votre DLL.
//
//   Pour éviter d'utiliser BORLNDMM.DLL, passez les chaînes comme paramètres "char *"
//   ou ShortString.
//
//   Si votre DLL utilise la version dynamique de la RTL, vous n'avez pas besoin
//   d'ajouter MEMMGR.LIB, car cela est fait automatiquement.
//---------------------------------------------------------------------------

#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
	return 1;
}
//---------------------------------------------------------------------------
