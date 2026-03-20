document.addEventListener('DOMContentLoaded', function() {
    fetch('/admin/api/chart_data')
        .then(response => response.json())
        .then(data => {
            if (document.getElementById('occupancyChart')) {
                const ctx = document.getElementById('occupancyChart').getContext('2d');

                // Gradient fill
                const gradient = ctx.createLinearGradient(0, 0, 0, 300);
                gradient.addColorStop(0, 'rgba(114, 9, 183, 0.8)');
                gradient.addColorStop(1, 'rgba(114, 9, 183, 0.1)');

                new Chart(ctx, {
                    type: 'bar',
                    data: {
                        labels: data.occupancy.labels,
                        datasets: [{
                            label: 'Vehicles Entered',
                            data: data.occupancy.data,
                            backgroundColor: gradient,
                            borderColor: '#7209b7',
                            borderWidth: 1,
                            borderRadius: 5
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        plugins: {
                            legend: { display: false },
                            tooltip: {
                                callbacks: {
                                    title: (items) => `Hour: ${items[0].label}`,
                                    label: (item) => ` ${item.raw} vehicle(s) entered`
                                }
                            }
                        },
                        scales: {
                            x: {
                                grid: { display: false },
                                ticks: { maxRotation: 45, minRotation: 45, font: { size: 10 } }
                            },
                            y: {
                                beginAtZero: true,
                                ticks: { precision: 0, stepSize: 1 },
                                grid: { color: 'rgba(150,150,150,0.15)' }
                            }
                        }
                    }
                });
            }
        })
        .catch(err => console.error('Error loading chart data:', err));
});
