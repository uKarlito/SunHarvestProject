const http = require('http');
const PORT = 3000;

let tick = 0;
let bombaLigada = false;
const historico = [];

function getEstado() {
  tick++;
  const umidade  = 20 + Math.abs(Math.sin(tick * 0.15)) * 70;
  const corrente = bombaLigada ? 3.5 + Math.sin(tick * 0.1) * 1.5 : 0.1 + Math.random() * 0.2;
  const tensao   = 24.0;
  const potencia = tensao * corrente;
  const efic     = bombaLigada ? Math.min((potencia / 1100) * 100, 100) : 0;
  const soloSeco = umidade < 30;
  const bombaFalha = bombaLigada && (corrente < 2.0 || corrente > 8.5 || efic < 40);

  let rec = '';
  if (soloSeco && !bombaFalha) rec = 'IRRIGAR - solo abaixo do limite critico';
  else if (bombaFalha)         rec = 'ALERTA - verificar bomba ou painel solar';
  else if (umidade > 60)       rec = 'OK - solo com umidade adequada';
  else                         rec = 'MONITORANDO - aguardando calculo ETo';

  const leitura = {
    umidade_solo_pct:     parseFloat(umidade.toFixed(1)),
    solo_seco:            soloSeco,
    corrente_bomba_a:     parseFloat(corrente.toFixed(2)),
    tensao_painel_v:      tensao,
    potencia_bomba_w:     parseFloat(potencia.toFixed(1)),
    eficiencia_bomba_pct: parseFloat(efic.toFixed(1)),
    bomba_falha:          bombaFalha,
    bomba_ligada:         bombaLigada,
    recomendacao_local:   rec,
    timestamp_ms:         tick * 3000
  };

  historico.push(leitura);
  if (historico.length > 10) historico.shift();
  return leitura;
}

const server = http.createServer((req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
  res.setHeader('Content-Type', 'application/json');

  if (req.method === 'OPTIONS') { res.writeHead(204); res.end(); return; }

  const url = req.url.split('?')[0];

  if (req.method === 'GET' && url === '/') {
    res.writeHead(200);
    res.end(JSON.stringify({
      device: 'SunHarvest-IoT-Node',
      farm_id: 'farm-equipe07-gs2026',
      crop_type: 'Soja', soil_type: 'Franco',
      area_ha: 5.0, panel_w: 3000, pump_w: 1100,
      status: 'online', uptime_s: tick * 3,
      bomba_ligada: bombaLigada
    }, null, 2));
    return;
  }

  if (req.method === 'GET' && url === '/leitura') {
    const estado = getEstado();
    console.log('[/leitura] Umidade: ' + estado.umidade_solo_pct + '% | Corrente: ' + estado.corrente_bomba_a + 'A | ' + estado.recomendacao_local);
    res.writeHead(200);
    res.end(JSON.stringify(estado, null, 2));
    return;
  }

  if (req.method === 'GET' && url === '/historico') {
    res.writeHead(200);
    res.end(JSON.stringify({
      farm_id: 'farm-equipe07-gs2026',
      total_leituras: historico.length,
      leituras: historico
    }, null, 2));
    return;
  }

  if (req.method === 'POST' && url === '/comando') {
    let body = '';
    req.on('data', function(chunk) { body += chunk; });
    req.on('end', function() {
      try {
        const parsed = JSON.parse(body);
        const acao = parsed.acao;
        if (acao === 'ligar_bomba') {
          bombaLigada = true;
          console.log('[/comando] Bomba LIGADA');
          res.writeHead(200);
          res.end(JSON.stringify({ status: 'bomba ligada' }));
        } else if (acao === 'desligar_bomba') {
          bombaLigada = false;
          console.log('[/comando] Bomba DESLIGADA');
          res.writeHead(200);
          res.end(JSON.stringify({ status: 'bomba desligada' }));
        } else {
          res.writeHead(400);
          res.end(JSON.stringify({ erro: 'acao desconhecida' }));
        }
      } catch(e) {
        res.writeHead(400);
        res.end(JSON.stringify({ erro: 'JSON invalido' }));
      }
    });
    return;
  }

  res.writeHead(404);
  res.end(JSON.stringify({ erro: 'endpoint nao encontrado' }));
});

server.listen(PORT, function() {
  console.log('');
  console.log('SunHarvest - Mock ESP32 Rodando!');
  console.log('Servidor: http://localhost:' + PORT);
  console.log('');
  console.log('Testa no navegador:');
  console.log('  http://localhost:' + PORT + '/');
  console.log('  http://localhost:' + PORT + '/leitura');
  console.log('  http://localhost:' + PORT + '/historico');
  console.log('');
  console.log('Aguardando requisicoes... (Ctrl+C para parar)');
});
