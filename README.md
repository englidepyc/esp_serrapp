# Per fare funzionare il tutto
## Per prima cosa 
Copia il file  `include/config.example.h` in un file chiamato `config.h` e inserisci le credenziali opportune, dopo potrai compilare ed eseguire il codice 
# Altri branch
I branch `feat/` sono fatti a partire dal develop e implementano piccole funzionalità, ad esempio lo shift register, il deep sleep etc e possono avere dei semplici esempi che li mostrano in azione in `src/main.cpp`.
<br>
Quando sei soddisfatto di avere sviluppato la tua piccola funzionalità nel tuo branch `test/`, caricalo con `git push --set-upstream origin test/tua_funzionalita`.
<br> Poi disctuiamo e proviamo e lo integriamo nel branch `develop`
# Documentazione
## Interfaccia hardware
I sensori e gli attuatori sono gestiti da interfacce hardware implementate in `src/real`. E' possibile cambiare l'environment PlatformIO da `release` a `test` per passare dalle vere implementazioni a quelle nella cartella `src/mocks` per sviluppare e testare senza hardware effettivo, non toccando i pin della board per evitare corti e malfunzionamenti, specialmente quando si settano i pin in modalità OUTPUT.

- E' NECESSARIO impostare correttamente come INPUT con `pinMode()` i pin di input del GALLEGGIANTE del serbatoio, del risveglio dal sesnore FUOCO e delle letture analogiche dell'acqua nel TERRENO, nell'apposita sezione di `setup()` nel `src/main.cpp`.

- E' NECSSARIO chiamare anche i vari metodi di setup delle restanti interfacce hardware, che inizializzano e testano le comunicazioni e impostano correttamente i pin. Questi sono `i2c_setup()` e `init_camera_system()`

## Deep sleep
La funzione `deep_sleep_setup()` viene chiamata prima di avviare lo sleep nel setup di `src/main.cpp` per impostare correttamente il tempo di sleep

## Controllo della serra
### Controllo proporzionale TPC (con deep sleep)
Nell'header `include/control/proportional.h` è possibile settare i vari parametri per il controllo tpc, che viene implementato nell'apposita sorgente e utilizzato da `src/taskMango.cpp`. Il ciclo a grandi linee è main -> taskMango -> taskComms con i dati -> main e deep sleep.