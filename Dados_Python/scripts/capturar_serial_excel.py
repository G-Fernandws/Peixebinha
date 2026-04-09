import serial
import pandas as pd
from datetime import datetime
import re
import math
import os

# =========================
# CONFIGURACOES DO ENSAIO
# =========================
PORTA_SERIAL = "COM7"   
BAUDRATE = 115200
TIMEOUT = 1

CABECALHO_ESPERADO = [
    "ciclo",
    "tempo_s",
    "temperatura_C",
    "pH",
    "EC25_uScm",
    "TDS_mgL",
    "NO3_mgL"
]

NOME_BASE_ARQUIVO = "teste_1"

CAMINHO_SAIDA = r"C:\Users\gegam\Repositorio_GIT\Peixebinha\Dados_Python\dados brutos"

# =========================
# FUNCOES AUXILIARES
# =========================
def agora_str():
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def nome_arquivo_saida():
    ts = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    return os.path.join(CAMINHO_SAIDA, f"{NOME_BASE_ARQUIVO}_{ts}.xlsx")

def tentar_converter_valor(valor):
    valor = valor.strip()

    if valor.lower() == "nan":
        return math.nan

    try:
        if re.fullmatch(r"-?\d+", valor):
            return int(valor)
        return float(valor)
    except Exception:
        return valor

def eh_linha_csv_dados(linha, n_colunas):
    partes = [p.strip() for p in linha.split(",")]
    return len(partes) == n_colunas

# =========================
# ESTRUTURAS DE ARMAZENAMENTO
# =========================
dados_brutos = []
log_serial = []

cabecalho_detectado = None
inicio_ensaio = datetime.now()

# Garante que a pasta de saída exista
os.makedirs(CAMINHO_SAIDA, exist_ok=True)

print("Abrindo porta serial...")
ser = serial.Serial(PORTA_SERIAL, BAUDRATE, timeout=TIMEOUT)
print(f"Conectado em {PORTA_SERIAL} a {BAUDRATE} bps.")
print("Iniciando captura. Para encerrar, pressione Ctrl+C.\n")

try:
    while True:
        linha_bytes = ser.readline()

        if not linha_bytes:
            continue

        linha = linha_bytes.decode("utf-8", errors="ignore").strip()

        if not linha:
            continue

        timestamp = agora_str()

        # Guarda tudo no log serial
        log_serial.append({
            "data_hora_pc": timestamp,
            "tipo_linha": "serial",
            "conteudo": linha
        })

        # Detecta cabecalho CSV
        if linha.replace(" ", "") == ",".join(CABECALHO_ESPERADO):
            cabecalho_detectado = CABECALHO_ESPERADO.copy()
            print(f"[CABECALHO CSV DETECTADO] {linha}")
            continue

        # Se o cabecalho foi detectado, tenta interpretar como linha de dados
        if cabecalho_detectado is not None and eh_linha_csv_dados(linha, len(cabecalho_detectado)):
            partes = [p.strip() for p in linha.split(",")]

            registro = {
                "data_hora_pc": timestamp
            }

            valido = True

            for col, val in zip(cabecalho_detectado, partes):
                convertido = tentar_converter_valor(val)
                registro[col] = convertido

            # Validação simples: a coluna ciclo deve ser numérica
            if not isinstance(registro["ciclo"], (int, float)):
                valido = False

            if valido:
                dados_brutos.append(registro)
                print(f"[DADO] {registro}")
            else:
                print(f"[LOG] {linha}")
        else:
            print(f"[LOG] {linha}")

except KeyboardInterrupt:
    print("\nEncerrando captura...")

finally:
    fim_ensaio = datetime.now()
    ser.close()

    # =========================
    # DATAFRAMES
    # =========================
    df_dados = pd.DataFrame(dados_brutos)
    df_log = pd.DataFrame(log_serial)

    resumo = {
        "inicio_ensaio": [inicio_ensaio.strftime("%Y-%m-%d %H:%M:%S")],
        "fim_ensaio": [fim_ensaio.strftime("%Y-%m-%d %H:%M:%S")],
        "duracao_s": [(fim_ensaio - inicio_ensaio).total_seconds()],
        "total_linhas_log": [len(log_serial)],
        "total_registros_dados": [len(dados_brutos)]
    }

    if not df_dados.empty:
        for col in ["temperatura_C", "pH", "EC25_uScm", "TDS_mgL", "NO3_mgL"]:
            if col in df_dados.columns:
                serie = pd.to_numeric(df_dados[col], errors="coerce")
                resumo[f"{col}_media"] = [serie.mean()]
                resumo[f"{col}_min"] = [serie.min()]
                resumo[f"{col}_max"] = [serie.max()]
                resumo[f"{col}_desvio_padrao"] = [serie.std()]

    df_resumo = pd.DataFrame(resumo)

    # =========================
    # EXPORTACAO PARA EXCEL
    # =========================
    arquivo_saida = nome_arquivo_saida()

    with pd.ExcelWriter(arquivo_saida, engine="openpyxl") as writer:
        df_dados.to_excel(writer, sheet_name="dados_brutos", index=False)
        df_log.to_excel(writer, sheet_name="log_serial", index=False)
        df_resumo.to_excel(writer, sheet_name="resumo_teste", index=False)

    print(f"Arquivo salvo com sucesso em:\n{arquivo_saida}")