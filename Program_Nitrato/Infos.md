**CALIBRAÇÃO SENSOR DE NITRATO**

PONTO	CONCENTRAÇÃO \[mg/L]	VOLUME DA SOLUÇÃO ESTOQUE (100mg/L) \[mL]	VOLUME ÁGUA DESTILADA \[mL]	VOLUME FINAL

1					0							0											100						100

2					25							25											75						100

3					50							50											50						100

4					75							75											25						100

5					100							100											0						100



Diluição simples:



C1\*V1 = C2\*V2



Onde:

C1 = 100 mg/L (estoque)

C2 = concentração desejada

V2 = 100 mL (volume final escolhido)



**===================================================================================================================**



**FLUXO DA PROGRAMAÇÃO DE CALIBRAÇÃO**



\[INÍCIO]

&#x20;  ↓

Inicializar sistema

(Serial, ADC, parâmetros)

&#x20;  ↓

Exibir instruções ao usuário

&#x20;  ↓

Definir concentrações padrão (> 0 mg/L)

&#x20;  ↓

Para cada ponto de calibração:

&#x20;  ↓

&#x20;┌──────────────────────────────┐

&#x20;│ Informar concentração atual                                           │

&#x20;│ (ex: 25, 50, 75, 100 mg/L)                                                  │

&#x20;└──────────────────────────────┘

&#x20;  ↓

Aguardar usuário (ENTER)

&#x20;  ↓

Sensor estabilizado?

&#x20;  ↓ (não)

Aguardar estabilização

&#x20;  ↓ (sim)

Realizar múltiplas leituras (n≈100)

&#x20;  ↓

Calcular média do ADC

&#x20;  ↓

Converter ADC → Tensão

&#x20;  ↓

Calcular log10(concentração)

&#x20;  ↓

Armazenar:

\[Tensão, log(C)]

&#x20;  ↓

Repetir para todos os pontos

&#x20;  ↓

Aplicar regressão linear

(Tensão vs log(C))

&#x20;  ↓

Obter coeficientes:

a (inclinação) e b (intercepto)

&#x20;  ↓

Calcular R² (opcional)

&#x20;  ↓

Exibir resultados:

\- Dados experimentais

\- Equação: V = a·log(C) + b

\- R²

&#x20;  ↓

Exibir equação inversa:

C = 10^((V - b)/a)

&#x20;  ↓

\[FIM]

