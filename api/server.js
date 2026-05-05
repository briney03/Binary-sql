require('dotenv').config();
const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;

// Configuración recomendada para despliegue
app.use(cors({
    origin: process.env.FRONTEND_URL || '*',
    methods: ['GET', 'POST']
}));
app.use(express.json());

// Ruta relativa al motor. Usamos variable de entorno o fallback a ../motor
const motorRelativePath = process.env.DB_ENGINE_PATH || '../motor';
const MOTOR_EXEC = path.resolve(__dirname, motorRelativePath);
const MOTOR_CWD = path.dirname(MOTOR_EXEC); // Ejecutamos desde el directorio del motor

app.post('/api/query', (req, res) => {
    const { db, query } = req.body;

    if (!query) {
        return res.status(400).json({ error: 'Falta la propiedad "query"' });
    }

    let commandsToRun = '';

    if (db) {
        commandsToRun += `USAR ${db}\n`;
    }

    commandsToRun += `${query}\n`;
    commandsToRun += `SALIR\n`;

    const motorProcess = spawn(MOTOR_EXEC, [], { cwd: MOTOR_CWD });

    let stdoutData = '';
    let stderrData = '';

    motorProcess.stdout.on('data', (data) => {
        stdoutData += data.toString();
    });

    motorProcess.stderr.on('data', (data) => {
        stderrData += data.toString();
    });

    motorProcess.on('close', (code) => {
        let cleanOutput = stdoutData
            .split('\n')
            .filter(line =>
                !line.includes('Motor de Base de Datos v0.3 Iniciado') &&
                !line.includes('Escribe \'exit\' para salir') &&
                !line.includes('=========================================') &&
                !line.includes('Cerrando el motor de base de datos... Adios.')
            )
            .join('\n')
            .replace(/mibd(:[a-zA-Z0-9_]+)?>/g, '')
            .trim();

        if (code !== 0 || stderrData) {
            return res.status(500).json({
                success: false,
                error: stderrData || cleanOutput || `Error exit code: ${code}`
            });
        }

        if (cleanOutput.toLowerCase().includes('error:')) {
            return res.status(400).json({
                success: false,
                error: cleanOutput
            });
        }

        return res.json({
            success: true,
            output: cleanOutput
        });
    });

    motorProcess.on('error', (err) => {
        res.status(500).json({ success: false, error: 'No se pudo iniciar el motor de DB. Verifica DB_ENGINE_PATH.' });
    });

    motorProcess.stdin.write(commandsToRun);
    motorProcess.stdin.end();
});

app.post('/api/create-db', (req, res) => {
    const { name } = req.body;

    if (!name) {
        return res.status(400).json({ error: 'Falta el nombre de la DB' });
    }

    const createProcess = spawn(MOTOR_EXEC, [], { cwd: MOTOR_CWD });
    let stdoutData = '';
    let stderrData = '';

    createProcess.stdout.on('data', (data) => { stdoutData += data.toString(); });
    createProcess.stderr.on('data', (data) => { stderrData += data.toString(); });

    createProcess.on('close', (code) => {
        const output = stdoutData.replace(/mibd(:[a-zA-Z0-9_]+)?>/g, '').trim();
        if (code === 0 || output.includes('creada') || output.includes('éxito')) {
            return res.json({ success: true, output });
        }
        return res.status(500).json({ success: false, error: stderrData || output });
    });

    createProcess.on('error', (err) => {
        res.status(500).json({ success: false, error: err.message });
    });

    createProcess.stdin.write(`CREAR ${name}\nSALIR\n`);
    createProcess.stdin.end();
});

app.listen(PORT, () => {
    console.log(`🚀 Bridge API corriendo en el puerto ${PORT}`);
    console.log(`🔗 Usando ejecutable de DB en: ${MOTOR_EXEC}`);
});
