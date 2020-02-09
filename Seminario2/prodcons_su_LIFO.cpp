// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_su_LIFO.cpp
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con múltiples productores y consumidores.
// Opcion LIFO (stack)
//
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

constexpr int
   num_items  = 40 ;     // número de items a producir/consumir

mutex
   mtx ;                 // mutex de escritura en pantalla
const int num_productoras = 5, num_consumidoras = 4; //Numero arbitrario de hebras
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items], // contadores de verificación: consumidos
   cont_prod_hebra[num_productoras];

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int num_hebra)
{
   static int contador [num_productoras] = {};
   
   if ( contador[num_hebra] == 0 && num_hebra != 0)
       contador[num_hebra] = num_items/num_productoras * num_hebra;
   
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador[num_hebra] << endl << flush ;
   mtx.unlock();
   cont_prod[contador[num_hebra]] ++ ;
   cont_prod_hebra[num_hebra]++;
   return contador[num_hebra]++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdConsSU : public HoareMonitor
{
 private:
 static const int           // constantes:
	num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
	buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
	primera_libre ;          //  indice de celda de la próxima inserción
 CondVar         // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdConsSU(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsSU::ProdConsSU(  )
{
	ocupadas = newCondVar();
	libres = newCondVar();
	primera_libre = 0 ;
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsSU::leer(  )
{
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if (primera_libre == 0 )
      ocupadas.wait();

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < primera_libre  );
   primera_libre-- ;
   const int valor = buffer[primera_libre] ;


   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsSU::escribir( int valor )
{
   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( primera_libre == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( primera_libre < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsSU> monitor, int num_hebra )
{
    int aProducir = num_items/num_productoras;
   for( unsigned i = num_hebra*aProducir; i < num_hebra*aProducir + aProducir ; i++ )
   {
      int valor = producir_dato( num_hebra );
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsSU> monitor )
{
   for( unsigned i = 0 ; i < num_items/num_consumidoras ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Varios prod/cons, Monitor SU, buffer LIFO)." << endl
        << "-------------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdConsSU> monitor = Create<ProdConsSU>();

   thread hebra_productora[num_productoras],
          hebra_consumidora[num_consumidoras];
          
    for (int i=0; i<num_productoras; i++)
        hebra_productora[i] = thread (funcion_hebra_productora, monitor, i);
    for (int i=0; i<num_consumidoras; i++)
        hebra_consumidora[i] = thread (funcion_hebra_consumidora, monitor);

    for (int i=0; i<num_productoras; i++)
        hebra_productora[i].join() ;
    for (int i=0; i<num_consumidoras; i++)
        hebra_consumidora[i].join() ;

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
