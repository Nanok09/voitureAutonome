#include <python2.7/Python.h>

int main(){
	PyObject *retour, *module, *fonction, *argument;
	char *resultat;
	Py_Initialize();
	
	PySys_SetPath(".");
	module =PyImport_ImportModule("TestPython");
	fonction = PyObject_GetAttrString (module, "somme");
	
	argument =Py_BuildValue("(i)",10);
	
	retour=PyEval_CallObject (fonction, argument);
	
	PyArg_Parse (retour, "i", &resultat);
	printf("Resultat du test Python :  %i \n",resultat);
	
	Py_Finalize();
	return 0;
}
	
