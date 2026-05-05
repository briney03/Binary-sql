const API = import.meta.env.VITE_API_URL || 'http://localhost:3000';

const tableList = document.getElementById('table-list');
const tableCount = document.getElementById('table-count');
const queryInput = document.getElementById('query-input');
const btnExecute = document.getElementById('btn-execute');
const btnClear = document.getElementById('btn-clear');
const resultsView = document.getElementById('results-view');
const messagesView = document.getElementById('messages-view');
const motorStatus = document.getElementById('motor-status');
const motorLabel = document.getElementById('motor-label');
const activeSchema = document.getElementById('active-schema');
const schemaStatus = document.getElementById('schema-status');
const lineNumbers = document.getElementById('line-numbers');
const currentTabName = document.getElementById('current-tab-name');
const tabsContainer = document.getElementById('tabs-container');
const tabResults = document.getElementById('tab-results');
const tabMessages = document.getElementById('tab-messages');
const execTimeValue = document.getElementById('exec-time-value');
const latencyValue = document.getElementById('latency-value');
const connectionStatus = document.getElementById('connection-status');
const btnNewDb = document.getElementById('btn-new-db');

let currentDb = '';
let tabCount = 1;
let queryHistory = [];
let historyIndex = -1;

function setStatus(connected, label) {
  motorStatus.className = connected
    ? 'w-2 h-2 rounded-full bg-emerald-500'
    : 'w-2 h-2 rounded-full bg-yellow-500';
  motorLabel.textContent = label;
  connectionStatus.innerHTML = connected
    ? `<span class="w-1.5 h-1.5 rounded-full bg-emerald-500"></span><span>Conectado</span>`
    : `<span class="w-1.5 h-1.5 rounded-full bg-yellow-500"></span><span>Offline</span>`;
}

function updateLineNumbers() {
  const lines = queryInput.value.split('\n').length;
  let html = '';
  for (let i = 1; i <= Math.max(lines, 1); i++) {
    html += `<div>${i}</div>`;
  }
  lineNumbers.innerHTML = html;
}

function addTab(name = null) {
  tabCount++;
  const tabName = name || `query_${tabCount}.sql`;
  const tab = document.createElement('div');
  tab.className = 'flex items-center gap-2 px-3 py-1.5 bg-surface-700 rounded-t-lg border-t border-l border-r border-surface-600';
  tab.innerHTML = `
    <span class="text-xs text-slate-400 font-mono">${tabName}</span>
    <button class="text-slate-500 hover:text-slate-300 text-lg leading-none">×</button>
  `;
  tab.querySelector('button').addEventListener('click', () => {
    if (tabsContainer.children.length > 1) {
      tab.remove();
    }
  });
  tabsContainer.appendChild(tab);
  currentTabName.textContent = tabName;
  return tabName;
}

function parseMotorOutput(output) {
  if (!output || typeof output !== 'string') return [];
  try {
    const lines = output.trim().split('\n').filter(l => l.trim());
    const result = [];

    for (const line of lines) {
      const trimmed = line.trim();
      // Ignorar cabeceras o mensajes
      if (!trimmed || trimmed.startsWith('Error') || trimmed.startsWith('ERROR') || trimmed.startsWith('---')) continue;

      // Si la línea tiene formato "columna: valor | columna2: valor2"
      if (trimmed.includes(':')) {
        const row = {};
        const pairs = trimmed.split('|').map(p => p.trim());
        let validPairs = 0;
        
        pairs.forEach(pair => {
            const splitIndex = pair.indexOf(':');
            if (splitIndex > -1) {
                const key = pair.substring(0, splitIndex).trim();
                const val = pair.substring(splitIndex + 1).trim();
                row[key] = val;
                validPairs++;
            }
        });
        
        if (validPairs > 0) {
            result.push(row);
        }
      }
    }
    return result;
  } catch (e) {
    console.error('Parse error:', e);
    return [];
  }
}

function renderResults(data, execTime) {
  execTimeValue.textContent = execTime ? `${execTime}ms` : '--ms';
  latencyValue.textContent = execTime ? `${execTime}ms` : '--ms';

  if (!data || (Array.isArray(data) && data.length === 0)) {
    resultsView.innerHTML = `
      <div class="flex flex-col items-center justify-center h-full text-slate-500 p-8">
        <svg class="w-14 h-14 mb-4 text-slate-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z"/>
        </svg>
        <p class="text-sm">Query ejecutada. Sin resultados.</p>
      </div>`;
    return;
  }

  if (typeof data === 'string') {
    if (data.toLowerCase().includes('error')) {
      resultsView.innerHTML = `
        <div class="p-4 m-4 bg-red-500/10 border border-red-500/30 rounded-lg">
          <div class="flex items-center gap-2 text-red-400 font-semibold mb-2">
            <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"/>
            </svg>
            Error
          </div>
          <pre class="text-red-300 text-sm whitespace-pre-wrap">${escapeHtml(data)}</pre>
        </div>`;
      return;
    }
    resultsView.innerHTML = `
      <div class="p-4 m-4">
        <pre class="text-emerald-400 text-sm whitespace-pre-wrap">${escapeHtml(data)}</pre>
      </div>`;
    return;
  }

  if (Array.isArray(data) && data.length > 0) {
    const columns = Object.keys(data[0]);
    let html = `<div class="overflow-auto h-full"><table class="w-full text-sm">`;
    html += `<thead class="bg-surface-700 sticky top-0"><tr>`;
    columns.forEach(col => {
      html += `<th class="px-4 py-3 text-left font-semibold text-accent-400 border-b border-surface-600 whitespace-nowrap">${escapeHtml(col)}</th>`;
    });
    html += `</tr></thead><tbody>`;
    data.forEach((row, i) => {
      html += `<tr class="${i % 2 === 0 ? 'bg-surface-700/30' : 'bg-surface-700/10'} hover:bg-surface-600/50 transition-colors">`;
      columns.forEach(col => {
        const val = row[col] === null ? '<span class="text-slate-600 italic">null</span>' : escapeHtml(String(row[col]));
        html += `<td class="px-4 py-2.5 border-b border-surface-600/50 font-mono whitespace-nowrap">${val}</td>`;
      });
      html += `</tr>`;
    });
    html += `</tbody></table></div>`;
    resultsView.innerHTML = html;
  } else {
    resultsView.innerHTML = `
      <div class="flex flex-col items-center justify-center h-full text-slate-500 p-8">
        <svg class="w-14 h-14 mb-4 text-slate-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M5 13l4 4L19 7"/>
        </svg>
        <p class="text-sm">Query ejecutada exitosamente.</p>
        <pre class="mt-3 text-emerald-400 text-xs bg-surface-700/50 p-3 rounded-lg max-w-xl overflow-auto">${JSON.stringify(data, null, 2)}</pre>
      </div>`;
  }
}

function addMessage(type, msg) {
  const msgEl = document.createElement('div');
  msgEl.className = type === 'error' ? 'text-red-400' : type === 'success' ? 'text-emerald-400' : 'text-slate-400';
  msgEl.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
  messagesView.appendChild(msgEl);
  messagesView.scrollTop = messagesView.scrollHeight;
}

function escapeHtml(str) {
  if (!str) return '';
  const div = document.createElement('div');
  div.textContent = str;
  return div.innerHTML;
}

tabResults.addEventListener('click', () => {
  tabResults.className = 'results-tab px-3 py-1.5 text-sm font-medium text-white border-b-2 border-accent-400';
  tabMessages.className = 'results-tab px-3 py-1.5 text-sm font-medium text-slate-400 hover:text-white transition-colors';
  resultsView.classList.remove('hidden');
  messagesView.classList.add('hidden');
});

tabMessages.addEventListener('click', () => {
  tabMessages.className = 'results-tab px-3 py-1.5 text-sm font-medium text-white border-b-2 border-accent-400';
  tabResults.className = 'results-tab px-3 py-1.5 text-sm font-medium text-slate-400 hover:text-white transition-colors';
  messagesView.classList.remove('hidden');
  resultsView.classList.add('hidden');
});

queryInput.addEventListener('input', updateLineNumbers);
queryInput.addEventListener('scroll', () => {
  lineNumbers.scrollTop = queryInput.scrollTop;
});

// Tab add button
const addTabBtn = document.getElementById('btn-add-tab');
if (addTabBtn) {
  addTabBtn.addEventListener('click', () => addTab());
}

document.querySelectorAll('.nav-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.nav-btn').forEach(b => {
      b.className = 'nav-btn w-full flex items-center gap-3 px-3 py-2 rounded-lg text-slate-300 hover:text-white hover:bg-surface-700 transition-colors text-left';
      b.classList.remove('text-white', 'bg-surface-700', 'border-l-2', 'border-accent-400');
    });
    btn.className = 'nav-btn w-full flex items-center gap-3 px-3 py-2 rounded-lg text-white bg-surface-700 border-l-2 border-accent-400 transition-colors text-left';
  });
});

// Nueva DB - ejecuta CREAR directamente
btnNewDb.addEventListener('click', async () => {
  const name = prompt('Nombre de la nueva DB:');
  if (!name) return;

  btnNewDb.disabled = true;
  btnNewDb.textContent = '...';

  try {
    const res = await fetch(`${API}/api/query`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ query: `CREAR ${name}` }),
    });
    const data = await res.json();

    if (data.success) {
      addMessage('success', `DB "${name}" creada`);
      currentDb = name;
      activeSchema.textContent = name;
      schemaStatus.className = 'w-2 h-2 rounded-full bg-emerald-500';
      setStatus(true, 'Conectado');
      loadDatabases();
      loadTables();
    } else {
      addMessage('error', data.error || 'Error creando DB');
    }
  } catch (e) {
    addMessage('error', `Error: ${e.message}`);
  } finally {
    btnNewDb.disabled = false;
    btnNewDb.textContent = 'Nueva DB';
  }
});

async function loadTables() {
  if (!currentDb) return;
  try {
    const res = await fetch(`${API}/api/query`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ db: currentDb, query: 'MOSTRAR TABLAS' }),
    });
    const data = await res.json();
    
    const output = data.output || '';
    const lines = output.split('\n').map(l => l.trim()).filter(l => l);
    
    const tableNames = [];
    let isTableList = false;
    
    for (const line of lines) {
      if (line.startsWith('--- Tablas')) {
        isTableList = true;
        continue;
      }
      if (isTableList) {
        if (!line.startsWith('=>') && !line.startsWith('Info:') && !line.startsWith('---')) {
          tableNames.push(line);
        }
      }
    }
    
    tableList.innerHTML = '';
    if (tableNames.length > 0) {
      tableCount.textContent = tableNames.length;
      tableNames.forEach(tableNameRaw => {
        const tableName = tableNameRaw.trim();
        const li = document.createElement('li');
        li.innerHTML = `
          <button class="w-full flex items-center gap-2 px-3 py-2 rounded-lg text-slate-300 hover:text-white hover:bg-surface-600 transition-colors text-left">
            <svg class="w-4 h-4 text-accent-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M3 10h18M3 14h18m-9-4v8m-7 0h14a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z"/>
            </svg>
            <span class="text-sm">${escapeHtml(tableName)}</span>
          </button>`;
        li.querySelector('button').addEventListener('click', () => {
          queryInput.value = `SELECCIONAR ${tableName}`;
          updateLineNumbers();
          currentTabName.textContent = `${tableName}.sql`;
        });
        tableList.appendChild(li);
      });
      addMessage('info', `${lines.length} tablas cargadas`);
    } else {
      tableCount.textContent = '0';
    }
  } catch (e) {
    addMessage('error', `Error cargando tablas: ${e.message}`);
  }
}

btnExecute.addEventListener('click', executeQuery);

async function executeQuery() {
  const query = queryInput.value.trim();
  if (!query) return;

  const startTime = Date.now();

  btnExecute.disabled = true;
  btnExecute.innerHTML = `
    <svg class="w-4 h-4 animate-spin" fill="none" viewBox="0 0 24 24">
      <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"></circle>
      <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z"></path>
    </svg>
    Ejecutando...`;

  addMessage('info', `Ejecutando: ${query.substring(0, 50)}${query.length > 50 ? '...' : ''}`);

  try {
    const res = await fetch(`${API}/api/query`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ db: currentDb, query }),
    });
    const data = await res.json();
    const execTime = Date.now() - startTime;

    if (data.error) {
      addMessage('error', data.error);
    }

    const result = data.success ? parseMotorOutput(data.output) : [];
    if (result.length > 0) {
      addMessage('success', `${result.length} filas retornadas en ${execTime}ms`);
    } else if (data.output) {
      addMessage('success', `Query ejecutada en ${execTime}ms`);
      renderResults(data.output, execTime);
      tabResults.click();
      
      // Auto-refresh si la query modifica estructura
      const upperQuery = query.toUpperCase();
      if (upperQuery.includes('TABLA')) loadTables();
      if (upperQuery.includes('BASE DE DATOS')) loadDatabases();
      
      return;
    } else {
      addMessage(data.error ? 'error' : 'success', data.error || `Query ejecutada en ${execTime}ms`);
    }

    renderResults(result.length > 0 ? result : (data.error || null), execTime);
    tabResults.click();


    queryHistory.unshift(query);
    historyIndex = -1;

  } catch (e) {
    addMessage('error', `Error de conexión: ${e.message}`);
    resultsView.innerHTML = `
      <div class="p-4 m-4 bg-red-500/10 border border-red-500/30 rounded-lg text-red-400">
        Error de conexión: ${e.message}
      </div>`;
  } finally {
    btnExecute.disabled = false;
    btnExecute.innerHTML = `
      <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.554z"/><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z"/></svg>
      Ejecutar`;
  }
}

btnClear.addEventListener('click', () => {
  queryInput.value = '';
  updateLineNumbers();
  resultsView.innerHTML = `
    <div class="flex flex-col items-center justify-center h-full text-slate-500 p-8">
      <svg class="w-14 h-14 mb-4 text-slate-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M4 7v10c0 2.21 3.582 4 8 4s8-1.79 8-4V7M4 7c0 2.21 3.582 4 8 4s8-1.79 8-4M4 7c0-2.21 3.582-4 8-4s8 1.79 8 4m-4 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16"/>
      </svg>
      <p class="text-sm">Ejecuta una query para ver los resultados</p>
    </div>`;
  messagesView.innerHTML = '<div class="text-slate-500">// Messages will appear here</div>';
  execTimeValue.textContent = '--ms';
  latencyValue.textContent = '--ms';
  currentTabName.textContent = 'query_1.sql';
});

queryInput.addEventListener('keydown', (e) => {
  if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
    executeQuery();
  }
  if (e.key === 'ArrowUp' && queryHistory.length > 0) {
    e.preventDefault();
    historyIndex = Math.min(historyIndex + 1, queryHistory.length - 1);
    queryInput.value = queryHistory[historyIndex];
    updateLineNumbers();
  }
  if (e.key === 'ArrowDown') {
    e.preventDefault();
    historyIndex = Math.max(historyIndex - 1, -1);
    queryInput.value = historyIndex === -1 ? '' : queryHistory[historyIndex];
    updateLineNumbers();
  }
});

setStatus(false, 'Offline');
addTab();
loadDatabases();

async function loadDatabases() {
  try {
    const res = await fetch(`${API}/api/query`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ query: 'MOSTRAR BASES DE DATOS' }),
    });
    const data = await res.json();
    const dbListEl = document.getElementById('db-list');
    const dbCountEl = document.getElementById('db-count');
    
    const output = data.output || '';
    const lines = output.split('\n').filter(l => l.trim() && !l.includes('---'));
    
    dbListEl.innerHTML = '';
    dbCountEl.textContent = lines.length;
    
    lines.forEach(db => {
      const dbName = db.trim();
      const li = document.createElement('li');
      li.innerHTML = `
        <button class="w-full flex items-center gap-2 px-3 py-2 rounded-lg text-slate-300 hover:text-white hover:bg-surface-600 transition-colors text-left">
          <svg class="w-4 h-4 text-accent-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
             <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 7v10c0 2.21 3.582 4 8 4s8-1.79 8-4V7M4 7c0 2.21 3.582 4 8 4s8-1.79 8-4M4 7c0-2.21 3.582-4 8-4s8 1.79 8 4"/>
          </svg>
          <span class="text-sm">${escapeHtml(dbName)}</span>
        </button>`;
      li.querySelector('button').addEventListener('click', () => {
        currentDb = dbName;
        document.getElementById('active-schema').textContent = dbName;
        document.getElementById('schema-status').className = 'w-2 h-2 rounded-full bg-emerald-500';
        setStatus(true, 'Conectado');
        loadTables();
      });
      dbListEl.appendChild(li);
    });
  } catch (e) {
    console.error("Error loading DBs", e);
  }
}