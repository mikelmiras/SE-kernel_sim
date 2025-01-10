Proiektuaren GitHub errepositorioa: [https://github.com/mikelmiras/SE-kernel_sim](https://github.com/mikelmiras/SE-kernel_sim)

# Sarrera

Txosten honetan deskribatutako proiektuaren helburua prozesuak planifikatzeko sistema bat ezartzea da, hainbat prozesuk *PUZ*a lortzeko lehiatzen duten giro multitarea simulatuko duena. Sistema horrek exekuzio-hari ugari (*thread*) erabiltzen ditu prozesuak kudeatzeko, *PUZ*a esleitzeko eta zehaztutako plangintza-politiken arabera ordena egokian gauzatzen direla ziurtatzeko.

Prozesuak planifikatzeko teknika ohikoenak aztertzen dira, hala nola First Come First Served (**FCFS**) eta Shortest Job First (**SJF**). Teknika horiek prozesuak efizientzia-irizpide desberdinen arabera antolatzeko erabiltzen dira. *FCFS* ikuspegi sinple bat da, non prozesuak iristen diren ordenan exekutatzen diren; *SJF*k, berriz, exekuzio-denbora laburragoak dituzten prozesuak lehenesten ditu. Testuinguru horretan, inplementatutako sistemak bi politikak simulatzeko gaitasuna du, eta aukera ematen du haien jarduna hainbat agertokitan alderatzeko.

Simulazioan, halaber, etendura-sistema bat kudeatzen da, erloju simulatu baten bidez. Erloju horrek gertaerak sortzen ditu tarte erregularretan, prozesuen egoera-trantsizioak aktibatzeko eta abiarazteko. Sistema hori hari anitzeko arkitektura erabiliz inplementatzen da, eta hariek *PUZ*ean exekutatzen diren prozesuak nahiz ekitaldien kudeaketaz, programazioaz eta prozesuen egoeren jarraipenaz arduratzen direnak irudikatzen dituzte.


# Sistemaren deskribapen orokorra

Garatutako sistema prozesuen plangintzaren simulazio bat da, ataza anitzeko sistema eragile batean. Ingurune horretan, prozesu ugari sortzen, kolatzen eta gauzatzen ditu *PUZ*ak, plangintza-politika baten arabera. Jarraian, sistema hau osatzen duten osagai guztiak deskribatuko ditugu.

1- **Prozesuak sortzea**: Sistemak prozesuak sortzeko hari bat du. Hari horrek prozesu berriak sortzen ditu tarte erregularretan, sistemaren konfigurazioan egokitzen den prozesuak sortzeko biderkatzaile baten arabera. Prozesu bakoitzak identifikatzaile bakarra (*PID*), exekuzio-denbora zenbatetsia (*burst time*) eta egoera bat ditu, **READY**, **RUNNING** edo **FINISHED** izan daitekeena. Prozesuak ausaz sortzen dira, tarte jakin bateko exekuzio-denborarekin.

2- **Prozesuak kolatzea**:

-   **Prozesu-ilara (*ProcessQueue*)**: Ilara honek gauzatzeko prest dauden prozesuak biltzen ditu, First Come, First Served (*FCFS*) plangintza-politikari jarraituz, non prozesuak iritsi ziren ordenan exekutatzen diren (FIFO datu egitura erabiliz).
- **Lehentasunezko prozesuen ilara (*PriorityProcessQueue*)**: Ilara hau Shortest Job First (*SJF*) politika erabiltzen denean erabiltzen da, non exekuzio-denbora laburragoa duten prozesuek lehentasun handiagoa duten lehenengo exekutatzeko.

Bi ilarak sinkronizazio-mekanismoek babesten dituzte, hala nola mutexek eta baldintzek. Horri esker, buztanetarako sarbide segurua bermatzen da hari anitzeko ingurune batean.

3- **Plangintza- eta gauzatze-hariak**: Egikaritze-hariek osatzen dute sistema, eta bakoitzak funtzio espezifiko bat du:
- **Erlojuaren haria (*Clock Thread*)**: Hari horrek doitasun handiko erloju bat simulatzen du, eta horrek gertaerak sortzen ditu tarte erregularretan, prozesuen exekuzioa sinkronizatzeko eta sistemaren aitzinamendua koordinatzeko.

- **Prozesuak sortzeko haria(*Process Generator Thread*)**: Prozesu berriak sortzeaz arduratzen da, konfiguratutako denbora-tartearen arabera.

- **Planifikatzailearen haria (*Scheduler Thread*)**: Hari hori arduratzen da eskura dauden langileei prozesuak esleitzeaz (*PUZaren hariak*), hautatutako plangintza-politika errespetatuz. Prozesuak eskuragarri badaude eta langileak libre badaude, planifikatzaileak prozesuak esleitzen dizkie, haien egoera eta langileen egoera eguneratuz.

- **Langileen hariak (*Worker Threads*)**: *PUZ*aren nukleoak irudikatzen dituzten hariak dira. Hari horietako bakoitzak prozesu bat gauzatzen du aldi berean, plangintza-politikaren arabera eskuragarri dagoen hurrengo prozesua hartuta. Langileak hiru egoeratan egon daitezke: **READY** (*exekutatzeko prest*),**RUNNING** (*exekutatzen*), edo **FINISHED** (*amaituta*).


4-**Tenporizadoreen erabilera**: Sistemak tenporizadore-hari bat (*Timer Thread*) erabiltzen du, eta sistemaren aitzinamendua koordinatzen du, gertaerak aldizka seinaleztatuz. Hari horri esker, exekuzio-hariek, hala nola planifikatzaileak edo langileek, une egokietan erreakzionatu ahal izango dute.

-**First Come, First Served (*FCFS*)**: Politika honetan, prozesuak iristen diren ordenan exekutatzen dira. Ez da lehentasun-kontsideraziorik egiten; beraz, sisteman sartzen den lehen prozesua exekutatzen lehena da.

-**Shortest Job First (*SJF*)**: Hemen, exekuzio-denbora laburrenak dituzten prozesuek lehentasuna dute exekutatuak izateko exekuzio-denbora luzeagoak dituzten prozesuek baino.

Bi politika horiek datu-egitura espezifikoen bidez (*ilarak*) ezartzen dira, eta prozesuak dagokion irizpidearen arabera kudeatzen dira.


5- **Harien sinkronizazioa**: Sistemak hainbat hari erabiltzen dituenez hainbat funtzio exekutatzeko, haien arteko sinkronizazioa funtsezkoa da. Mutexeak eta baldintza-aldagaiak (*kondizio aldakorrak*) erabiltzen dira baliabide partekatuak, hala nola prozesu-ilarak eta erloju-seinaleak, modu seguruan eta efizientean kudeatuko direla bermatzeko.

6- **Sistemaren konfigurazioa**: Sistema oso konfiguragarria da konfigurazio-fitxategi baten bidez. Aukera konfiguragarrien artean daude *PUZ*en nukleo kopurua (*lan-hariak*), erlojuaren maiztasuna (*milisegundotan*), prozesuak sortzeko biderkatzailea (*sortutako prozesu kopurua zehazten du*) eta erabili beharreko plangintza-politika (*FCFS edo SJF*). Parametro horiek konfigurazio-fitxategi batetik irakurtzen dira, eta hasten direnean aplikatzen zaizkio sistemari.

Diseinu modular horri esker, sistema erraz egokitu daiteke simulazio-agertokietara, eta plangintza-politiken arteko konparazio-probak egin daitezke.

# Sistemaren diseinua eta inplementazioa
Sistemaren inplementazioak ikuspegi modularra eta harietara bideratua jarraitzen du, non sistemako osagai bakoitza modu independentean kudeatzen den, baina batera lan eginez. Jarraian, sistemaren logika inplementatzen duten kodearen funtsezko funtzioak zehazten dira.

1- **Prozesuen sorrera (*generate_processes*)**:

- "Generate_processes (*)" funtzioa prozesuak ausaz sortzeaz arduratzen da, identifikadore bakar bat (*PID*) eta tarte konfiguragarri baten barruan zenbatetsitako exekuzio-denbora esleituz.

- Sortutako prozesuak dagokion ilarara bidaltzen dira, hautatu den plangintza-politikaren arabera. Oraingo bertsioan,**First Come, First Served** (*FCFS*) politika erabiltzen bada, prozesuak ProcessQueue sisteman sartzen dira. **Shortest Job First** (*SJF*) politika hautatzen bada, "*PriorityProcessQueue*" en sartzen dira.

- Funtzioa aldizka exekutatzen da hari batean, eta horrek prozesuak tarte erregularretan sortzen direla ziurtatzen du.

2- **Prozesuak kolatzea (*enqueue_process*)**:

- "Enqueue_process (*)" funtzioa sortutako prozesuak dagokion ilaran txertatzeaz arduratzen da. Plangintza-politikaren arabera, prozesuak nahi den gauzatze-ordena errespetatzeko moduan gehitzen dira.

- Plangintza-politika**FCFS** bada, prozesuak "*ProcessQueue*" delakoari gehitzen zaizkio sekuentzialki. *SJF*rako, prozesuak "*PriorityProcessQueue*"n txertatzen dira, gauzatze-denboraren arabera, eta prozesua ahalik eta denbora laburrenean ilararen buruan egongo dela ziurtatzen da.

3- **Plangintza (*scheduler_thread*)**:

- "Scheduler_thread (*)" funtzioak planifikatzailearen portaera inplementatzen du. Hari horrek kontrolatzen du zer prozesu exekutatuko den ondoren. Prozesuak exekutatzeko prest badaude eta langileak prest badaude, planifikatzaileak prozesu bat esleitzen dio eskuragarri dagoen lan-hari honi.

- Funtzio honetan, *FCFS* eta *SJF* plangintza-politikak ezartzen dira. *FCFS* rako, planifikatzaileak "*ProcessQueue*" ilarako lehen prozesua esleitzen du. *SJF* rako, planifikatzaileak prozesua hautatzen du, *PriorityProcessQueue* gauzatzeko denborarik laburrenarekin.

- Gainera, planifikatzaileak mutexeak eta baldintza-aldagaiak erabiliz sinkronizatzen ditu hariak, baliabideak modu seguruan eta eraginkorrean partekatuko direla bermatuz.

4- **Prozesuak gauzatzea (*worker_thread*)**:

- "Worker_thread (*)" funtzioak *PUZ*aren nukleoak adierazten ditu. Lan-hari bakoitzak ilararen prozesu bat hartzen du (*planifikatzaileak esleitutakoaren arabera*) eta exekutatu egiten du.

- Prozesu baten exekuzioa simulatzeko, prozesua gauzatzeko aurreikusitako denborarekiko proportzionala den atzerapen bat erabiltzen da ("*usleep (*)*" -ekin simulatua), eta prozesua **READY**, **RUNNING**, eta **FINISHED** "egoeretatik igarotzen da.

- Exekuzioan zehar, sinkronizazioa egiten da hariak baliabide partekatuetara modu gatazkatsuan iristen ez direla bermatzeko.

5- **Tenporizadore (*timer_thread*)**:

- "Timer_thread (*)" funtzioak sistemaren erlojua simulatzen du eta tarte erregularretan exekutatzen da, gainerako hariak ekintza bat egin behar dutenean seinalatuz.

- Hari hori funtsezkoa da exekuzio-fluxua sinkronian mantentzeko. Aldizkako seinaleak igortzen ditu prozesuak sortzeko eta plangintzak aurrera egiteko, sistemako osagai bakoitzak bere funtzioak une egokian gauzatu ahal izango dituela ziurtatuz.

6- **Konfigurazio-interfazea (*get_config_value*)**:
- Funtzio honek konfigurazio-parametroak kanpoko fitxategi batetik irakurtzen ditu. Parametro horien artean daude:
    -  Erabilgarri dauden CPU nukleoen kopurua (*lan-hariak*).
    - Prozesuak sortzen diren arteko denbora-tartea.
    - Erabili beharreko plangintza-politika (*FCFS edo SJF*).
    - Erlojuaren maiztasuna (*milisegundotan*).

- Balio horiek sistema konfiguratzeko eta haren portaera zuzenean aldatu beharrik gabe aldatzeko erabiltzen dira.

7- **Sinkronizazioa (*mutexak eta baldintza-aldagaiak*)**:

- Harien konkurrentzia maneiatzeko eta ibiltarte-baldintzak saihesteko, sistemak hainbat sinkronizazio-teknika erabiltzen ditu, hala nola mutexak (*honako mutex_pthread_mutex_t*) eta baldintza-aldagaiak.

- Adibidez, prozesu-ilarek manipulatzen dituzten funtzioek mutexak erabiltzen dituzte prozesu bat aldi berean hari ugariren bidez kolatzen edo askatzen ez dela ziurtatzeko. Baldintza-aldagaiak hariei jakinarazteko erabiltzen dira prozesu bat gauzatzeko unea denean edo ilaran artatu beharreko prozesu berriak daudenean.
