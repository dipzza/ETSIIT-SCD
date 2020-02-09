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

class Barberia : public HoareMonitor
{
	private:
		CondVar durmiendo, salaDeEspera, sillaDeCorte;

	public:                    // constructor y métodos públicos
		Barberia() ;           // constructor
		void cortarPelo(int num_cliente);
		void siguienteCliente();
		void finCliente();
} ;

Barberia::Barberia()
{
	durmiendo = newCondVar();
	salaDeEspera = newCondVar();
	sillaDeCorte = newCondVar();
}

void Barberia::cortarPelo(int num_cliente)
{
	if (!durmiendo.empty())
	{
		cout << "Cliente " << num_cliente << " despierta al Barbero." << endl;
		durmiendo.signal();
	}
	else
	{
		cout << "Cliente " << num_cliente << " entra en la sala de espera." << endl;
		salaDeEspera.wait();
	}
	
	cout << "Cliente " << num_cliente << " se sienta en la silla de corte." << endl;
	sillaDeCorte.wait();
	cout << "Cliente " << num_cliente << " sale de la barberia." << endl;
}

void Barberia::siguienteCliente()
{
	if (salaDeEspera.empty())
	{
		cout << "Barbero se va a dormir" << endl;
		durmiendo.wait();
	} 
	salaDeEspera.signal();
}

void Barberia::finCliente()
{
	cout << "Barbero indica al cliente que puede salir" << endl;
	sillaDeCorte.signal();
}


void esperaCortarPelo()
{
	chrono::milliseconds duracion_esperar( aleatorio<20,100>() );
	
	cout << "Barbero empieza a cortar el pelo al cliente." << endl;
	this_thread::sleep_for(duracion_esperar);
    cout << "Barbero termina de cortar el pelo." << endl;
}

void esperarFueraBarberia(int num_cliente)
{
	chrono::milliseconds duracion_esperar( aleatorio<100,200>() );
	
	cout << "Cliente " << num_cliente << " empieza a esperar fuera de la barberia." << endl;
	this_thread::sleep_for(duracion_esperar);
    cout << "Cliente " << num_cliente << " vuelve a la barberia." << endl;
}


void hebra_barbero(MRef<Barberia> monitor)
{
    int ingrediente;
    
    while (true)
    {
        monitor->siguienteCliente();
        esperaCortarPelo();
		monitor->finCliente();
    }
}

void  hebra_cliente(MRef<Barberia> monitor, int num_cliente)
{
   while(true)
   {
       monitor->cortarPelo(num_cliente);
       esperarFueraBarberia(num_cliente);
   }
}

//----------------------------------------------------------------------

int main()
{
	MRef<Barberia> monitor = Create<Barberia>();
    thread barbero (hebra_barbero, monitor);
    thread cliente[3];
    
    for (int i=0; i<3; i++)
        cliente[i] = thread (hebra_cliente, monitor, i);
            
    barbero.join();
    for (int i=0; i<3; i++)
        cliente[i].join();
}
