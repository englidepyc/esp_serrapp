# Per fare funzionare il tutto
## Per prima cosa 
Copia il file  `include/config.example.h` in un file chiamato `config.h` e inserisci le credenziali opportune, dopo potrai compilare ed eseguire il codice 
# Altri branch
I branch `test/` sono fatti a partire dal develop e implementano piccole funzionalità, ad esempio lo shift register, il deep sleep etc e hanno dei semplici esempi che li mostrano in azione in `src/main`.
<br>
Quando sei soddisfatto di avere sviluppato la tua piccola funzionalità nel tuo branch `test/`, caricalo con `git push --set-upstream origin test/tua_funzionalita`.
<br> Poi disctuiamo e proviamo e lo integriamo nel branch `develop`