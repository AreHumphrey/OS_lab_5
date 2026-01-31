class TemperatureDashboard {
    constructor() {
        this.apiUrl = '/api';
        this.chartCtx = document.getElementById('chart').getContext('2d');
        this.chart = null;
        this.lastUpdate = null;
        
        this.initEventListeners();
        this.fetchCurrent();
        this.fetchStats();
        
        // Автообновление каждые 5 секунд
        setInterval(() => {
            this.fetchCurrent();
            this.fetchStats();
        }, 5000);
    }
    
    initEventListeners() {
        document.getElementById('refresh').addEventListener('click', () => {
            this.fetchCurrent();
            this.fetchStats();
        });
        
        document.getElementById('period').addEventListener('change', () => {
            this.fetchStats();
        });
        
        document.getElementById('bucket').addEventListener('change', () => {
            this.fetchStats();
        });
    }
    
    async fetchCurrent() {
        try {
            const response = await fetch(`${this.apiUrl}/current`);
            if (!response.ok) throw new Error('Failed to fetch current temp');
            
            const data = await response.json();
            this.updateCurrentTemp(data.value, data.epoch_ms);
            this.updateServerStatus('online');
        } catch (error) {
            console.error('Error fetching current temp:', error);
            this.updateServerStatus('offline');
        }
    }
    
    async fetchStats() {
        try {
            const period = document.getElementById('period').value;
            const bucket = document.getElementById('bucket').value;
            const end = Date.now();
            const start = end - parseInt(period);
            
            const url = `${this.apiUrl}/stats?bucket=${bucket}&start=${start}&end=${end}`;
            const response = await fetch(url);
            if (!response.ok) throw new Error('Failed to fetch stats');
            
            const data = await response.json();
            this.renderChart(data.data || []);
            this.renderTable(data.data || []);
            this.lastUpdate = new Date();
            document.getElementById('lastUpdate').textContent = this.lastUpdate.toLocaleTimeString();
        } catch (error) {
            console.error('Error fetching stats:', error);
            this.renderTable([]);
        }
    }
    
    updateCurrentTemp(value, timestamp) {
        const element = document.getElementById('currentTemp');
        if (value !== undefined) {
            element.textContent = `${value.toFixed(2)} °C`;
            element.className = ''; // Убираем анимацию загрузки
        } else {
            element.textContent = 'Нет данных';
            element.classList.add('loading');
        }
    }
    
    updateServerStatus(status) {
        const element = document.getElementById('serverStatus');
        element.textContent = status === 'online' ? 'Онлайн' : 'Оффлайн';
        element.className = status === 'online' ? 'online' : 'offline';
    }
    
    renderChart(data) {
        // Удаляем старый график если есть
        if (this.chart) {
            this.chart.destroy();
        }
        
        if (data.length === 0) {
            this.chartCtx.clearRect(0, 0, this.chartCtx.canvas.width, this.chartCtx.canvas.height);
            this.chartCtx.fillStyle = '#94a3b8';
            this.chartCtx.font = '20px Arial';
            this.chartCtx.textAlign = 'center';
            this.chartCtx.fillText('Нет данных для отображения', this.chartCtx.canvas.width / 2, this.chartCtx.canvas.height / 2);
            return;
        }
        
        // Подготовка данных для графика
        const labels = data.map(item => {
            const date = new Date(item[0]);
            return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        });
        const values = data.map(item => item[1]);
        
        // Создание простого линейного графика вручную (без библиотек)
        this.drawSimpleChart(labels, values);
    }
    
    drawSimpleChart(labels, values) {
        const ctx = this.chartCtx;
        const width = ctx.canvas.width;
        const height = ctx.canvas.height;
        
        // Очистка холста
        ctx.clearRect(0, 0, width, height);
        
        // Настройки
        const padding = 40;
        const chartWidth = width - padding * 2;
        const chartHeight = height - padding * 2;
        
        // Мин/макс значения
        const minVal = Math.min(...values);
        const maxVal = Math.max(...values);
        const range = maxVal - minVal || 1;
        
        // Рисуем сетку
        ctx.strokeStyle = '#334155';
        ctx.lineWidth = 1;
        
        // Горизонтальные линии
        for (let i = 0; i <= 5; i++) {
            const y = padding + chartHeight - (chartHeight * i / 5);
            ctx.beginPath();
            ctx.moveTo(padding, y);
            ctx.lineTo(width - padding, y);
            ctx.stroke();
            
            // Подписи значений
            ctx.fillStyle = '#94a3b8';
            ctx.font = '12px Arial';
            ctx.fillText(minVal + (range * i / 5).toFixed(1), padding - 30, y + 4);
        }
        
        // Рисуем линию графика
        ctx.strokeStyle = '#22d3ee';
        ctx.lineWidth = 3;
        ctx.beginPath();
        
        values.forEach((value, index) => {
            const x = padding + (chartWidth * index / Math.max(1, values.length - 1));
            const y = padding + chartHeight - ((value - minVal) / range * chartHeight);
            
            if (index === 0) {
                ctx.moveTo(x, y);
            } else {
                ctx.lineTo(x, y);
            }
            
            // Точки данных
            ctx.fillStyle = '#22d3ee';
            ctx.beginPath();
            ctx.arc(x, y, 4, 0, Math.PI * 2);
            ctx.fill();
        });
        
        ctx.stroke();
        
        // Подписи по оси X (время)
        ctx.fillStyle = '#94a3b8';
        ctx.font = '10px Arial';
        ctx.textAlign = 'center';
        
        labels.forEach((label, index) => {
            if (index % Math.max(1, Math.floor(labels.length / 10)) === 0) {
                const x = padding + (chartWidth * index / Math.max(1, labels.length - 1));
                ctx.fillText(label, x, height - padding + 15);
            }
        });
        
        // Заголовок оси Y
        ctx.save();
        ctx.translate(15, height / 2);
        ctx.rotate(-Math.PI / 2);
        ctx.fillStyle = '#94a3b8';
        ctx.font = '14px Arial';
        ctx.textAlign = 'center';
        ctx.fillText('Температура (°C)', 0, 0);
        ctx.restore();
    }
    
    renderTable(data) {
        const tbody = document.querySelector('#dataTable tbody');
        tbody.innerHTML = '';
        
        if (data.length === 0) {
            tbody.innerHTML = '<tr><td colspan="2">Нет данных</td></tr>';
            return;
        }
        
        // Сортируем по убыванию времени
        data.sort((a, b) => b[0] - a[0]);
        
        // Показываем последние 20 записей
        const displayData = data.slice(0, 20);
        
        displayData.forEach(item => {
            const row = document.createElement('tr');
            const timeCell = document.createElement('td');
            const tempCell = document.createElement('td');
            
            const date = new Date(item[0]);
            timeCell.textContent = date.toLocaleString('ru-RU');
            tempCell.textContent = `${item[1].toFixed(2)} °C`;
            tempCell.style.fontWeight = 'bold';
            tempCell.style.color = item[1] > 25 ? '#ef4444' : item[1] < 0 ? '#3b82f6' : '#10b981';
            
            row.appendChild(timeCell);
            row.appendChild(tempCell);
            tbody.appendChild(row);
        });
    }
}

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
    new TemperatureDashboard();
});
