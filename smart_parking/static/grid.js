// static/grid.js
// Polls /api/slots and dynamically builds the parking lot grid.

document.addEventListener('DOMContentLoaded', () => {
    const gridContainer = document.getElementById('liveGrid');
    
    // We check if we have a currently logged in user from a global var defined in the template
    const userRfid = window.currentUserRfid || null;

    async function fetchAndRenderSlots() {
        try {
            const response = await fetch('/api/slots');
            if (!response.ok) throw new Error('Failed to fetch slots');
            const data = await response.json();
            
            // Clear or update grid
            if (gridContainer) {
                gridContainer.innerHTML = '';
                
                data.slots.forEach(slot => {
                    const slotDiv = document.createElement('div');
                    slotDiv.className = 'parking-slot';
                    
                    // Determine status class
                    if (slot.occupied) {
                        if (userRfid && slot.rfid === userRfid) {
                            slotDiv.classList.add('slot-mine');
                        } else {
                            slotDiv.classList.add('slot-occupied');
                        }
                    } else {
                        slotDiv.classList.add('slot-free');
                    }
                    
                    // Label
                    const labelSpan = document.createElement('span');
                    labelSpan.className = 'slot-label';
                    labelSpan.innerText = `#${slot.slot_id}`;
                    slotDiv.appendChild(labelSpan);
                    
                    // Optional RFID display if occupied
                    if (slot.occupied && slot.rfid) {
                        const rfidSpan = document.createElement('span');
                        rfidSpan.className = 'rfid-label';
                        rfidSpan.innerText = slot.rfid;
                        slotDiv.appendChild(rfidSpan);
                    }
                    
                    gridContainer.appendChild(slotDiv);
                });
            }
        } catch (error) {
            console.error('Grid sync error:', error);
        }
    }

    // Initial fetch
    fetchAndRenderSlots();

    // Poll every 2 seconds for live feeling
    setInterval(fetchAndRenderSlots, 2000);
});
