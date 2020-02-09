#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

class Estanco : public HoareMonitor
{
	private:
		int ingr_disp = -1;
		CondVar mostradorVacio, ingredienteProducido[3];

	public:                    // constructor y métodos públicos
		Estanco() ;           // constructor
		void obtenerIngrediente(int num_fumador);
		void ponerIngrediente(int ingrediente);
		void esperarRecogidaIngrediente();
} ;

Estanco::Estanco()
{
	mostradorVacio = newCondVar();
	
	for (int i=0; i < 3; i++)
		ingredienteProducido[i] = newCondVar();
}

void Estanco::obtenerIngrediente(int num_fumador)
{
	if (ingr_disp != num_fumador)
		ingredienteProducido[num_fumador].wait();
	
	cout << "Retirado ingrediente " << num_fumador << endl;
	ingr_disp = -1;
	mostradorVacio.signal();
}

void Estanco::ponerIngrediente(int ingrediente)
{
	ingr_disp = ingrediente;
	cout << "Puesto ingrediente " << ingrediente << endl;
	ingredienteProducido[ingrediente].signal();
}

void Estanco::esperarRecogidaIngrediente()
{
	if (ingr_disp != -1)
		mostradorVacio.wait();
}

//----------------------------------------------------------------------
// Función que produce el ingrediente de un fumador
int producir()
{
    int ingr_random = aleatorio<0,2>();
    
    this_thread::sleep_for( chrono::milliseconds( aleatorio<20,200>() ));
    
    return ingr_random;
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
    int ingrediente;
    
    while (true)
    {
        ingrediente = producir();
        monitor->ponerIngrediente (ingrediente);
		monitor->esperarRecogidaIngrediente();
    }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador(MRef<Estanco> monitor, int num_fumador)
{
   while( true )
   {
       monitor->obtenerIngrediente( num_fumador );
       fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
	MRef<Estanco> monitor = Create<Estanco>();
    thread hebra_estanquero (funcion_hebra_estanquero, monitor);
    thread hebra_fumador[3];
    
    for (int i=0; i<3; i++)
        hebra_fumador[i] = thread (funcion_hebra_fumador, monitor, i);
            
    hebra_estanquero.join();
    for (int i=0; i<3; i++)
        hebra_fumador[i].join();
}
