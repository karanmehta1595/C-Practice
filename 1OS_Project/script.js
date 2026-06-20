function updateClock() {
    const clockElement = document.getElementById('live-clock');
    if (!clockElement) return;
    const now = new Date();
    let hours = now.getHours();
    const minutes = now.getMinutes();
    const ampm = hours >= 12 ? 'PM' : 'AM';
    hours = hours % 12 || 12; 
    clockElement.textContent = `${hours}:${minutes < 10 ? '0' + minutes : minutes} ${ampm}`;
}

document.addEventListener('DOMContentLoaded', () => {
    updateClock();
    setInterval(updateClock, 1000);

    const homeButton = document.getElementById('dock-home');
    const homeMenu = document.getElementById('home-menu');
    const desktopCanvas = document.getElementById('desktop-canvas');
    const bottomDock = document.getElementById('main-drop-dock');
    const appsGrid = document.getElementById('launcher-apps-grid');
    const squireTrigger = document.getElementById('squire-panel-trigger');
    const masterPanel = document.getElementById('master-dropdown-panel');

    if (homeButton && homeMenu) {
        homeButton.addEventListener('click', (e) => {
            e.stopPropagation();
            homeMenu.classList.toggle('state-closed');
        });
        document.addEventListener('click', (e) => {
            if (!homeMenu.classList.contains('state-closed') && !homeMenu.contains(e.target)) {
                homeMenu.classList.add('state-closed');
                if (masterPanel) masterPanel.classList.add('hidden'); 
            }
        });
    }

    if (squireTrigger && masterPanel) {
        squireTrigger.addEventListener('click', (e) => {
            e.stopPropagation();
            masterPanel.classList.toggle('hidden');
        });
        masterPanel.addEventListener('click', (e) => e.stopPropagation());
    }

    document.addEventListener('click', () => {
        if (masterPanel) masterPanel.classList.add('hidden');
    });

    document.getElementById('squire-animation-group')?.addEventListener('click', (e) => {
        const item = e.target.closest('.drop-item');
        if (!item || !homeMenu) return;
        document.querySelectorAll('#squire-animation-group .drop-item').forEach(el => el.classList.remove('active'));
        item.classList.add('active');
        homeMenu.setAttribute('data-animation', item.getAttribute('data-value'));
    });

    document.getElementById('squire-view-group')?.addEventListener('click', (e) => {
        const item = e.target.closest('.drop-item');
        if (!item || !appsGrid) return;
        document.querySelectorAll('#squire-view-group .drop-item').forEach(el => el.classList.remove('active'));
        item.classList.add('active');
        appsGrid.className = `apps-grid layout-${item.getAttribute('data-view')}`;
    });

    document.getElementById('squire-sort-group')?.addEventListener('click', (e) => {
        const item = e.target.closest('.drop-item');
        if (!item || !appsGrid) return;
        document.querySelectorAll('#squire-sort-group .drop-item').forEach(el => el.classList.remove('active'));
        item.classList.add('active');
        const strategy = item.getAttribute('data-sort');
        const elementNodes = [...appsGrid.querySelectorAll('.grid-item')];
        elementNodes.sort((nodeA, nodeB) => {
            if (strategy === 'name') return nodeA.getAttribute('data-app-title').localeCompare(nodeB.getAttribute('data-app-title'));
            if (strategy === 'size') return parseFloat(nodeB.getAttribute('data-app-size')) - parseFloat(nodeA.getAttribute('data-app-size'));
            if (strategy === 'date') return new Date(nodeB.getAttribute('data-app-date')) - new Date(nodeA.getAttribute('data-app-date'));
            return 0;
        });
        elementNodes.forEach(node => appsGrid.appendChild(node));
    });

    const alphaSlider = document.getElementById('launcher-opacity-slider');
    const alphaLabel = document.getElementById('opacity-pct-label');
    alphaSlider?.addEventListener('input', (e) => {
        const currentVal = e.target.value;
        if(alphaLabel) alphaLabel.textContent = `${currentVal}%`;
        if(homeMenu) homeMenu.style.setProperty('--launcher-opacity', currentVal / 100);
    });

    function initializeDragBus() {
        document.querySelectorAll('.grid-item').forEach(item => {
            item.addEventListener('dragstart', (e) => {
                e.dataTransfer.setData('source', 'launcher');
                e.dataTransfer.setData('app-id', item.getAttribute('data-app-id'));
                e.dataTransfer.setData('app-title', item.getAttribute('data-app-title'));
                e.dataTransfer.setData('app-img', item.getAttribute('data-app-img'));
            });
        });
        document.querySelectorAll('.pinned-app').forEach(item => {
            item.addEventListener('dragstart', (e) => {
                e.dataTransfer.setData('source', 'dock');
                e.dataTransfer.setData('element-id', item.id);
            });
            item.addEventListener('dragover', (e) => e.preventDefault());
        });
    }
    initializeDragBus();

    function getDropPosition(dock, mouseX) {
        const elements = [...dock.querySelectorAll('.pinned-app:not(.static-icon)')];
        return elements.reduce((closest, child) => {
            const box = child.getBoundingClientRect();
            const offset = mouseX - box.left - box.width / 2;
            return (offset < 0 && offset > closest.offset) ? { offset, element: child } : closest;
        }, { offset: Number.NEGATIVE_INFINITY }).element;
    }

    if (bottomDock) {
        bottomDock.addEventListener('dragover', (e) => { e.preventDefault(); e.stopPropagation(); bottomDock.classList.add('drag-over'); });
        bottomDock.addEventListener('dragleave', () => bottomDock.classList.remove('drag-over'));
        bottomDock.addEventListener('drop', (e) => {
            e.preventDefault(); e.stopPropagation(); bottomDock.classList.remove('drag-over');
            const source = e.dataTransfer.getData('source');
            const targetSibling = getDropPosition(bottomDock, e.clientX);
            if (source === 'dock') {
                const elementId = e.dataTransfer.getData('element-id');
                const movingNode = document.getElementById(elementId);
                if (movingNode) targetSibling ? bottomDock.insertBefore(movingNode, targetSibling) : bottomDock.appendChild(movingNode);
            } else if (source === 'launcher') {
                const appId = e.dataTransfer.getData('app-id');
                const appTitle = e.dataTransfer.getData('app-title');
                const appImg = e.dataTransfer.getData('app-img');
                if (!appId || document.getElementById(`pinned-${appId}`)) return;
                const newDockIcon = document.createElement('div');
                newDockIcon.className = 'app-icon pinned-app';
                newDockIcon.id = `pinned-${appId}`;
                newDockIcon.setAttribute('draggable', 'true');
                newDockIcon.setAttribute('data-app-id', appId);
                newDockIcon.title = appTitle;
                const img = document.createElement('img'); img.src = appImg; img.alt = appTitle;
                newDockIcon.appendChild(img);
                targetSibling ? bottomDock.insertBefore(newDockIcon, targetSibling) : bottomDock.appendChild(newDockIcon);
                if (homeMenu) homeMenu.classList.add('state-closed');
            }
            initializeDragBus();
        });
    }

    if (desktopCanvas) {
        desktopCanvas.addEventListener('dragover', (e) => e.preventDefault());
        desktopCanvas.addEventListener('drop', (e) => {
            const source = e.dataTransfer.getData('source');
            const elementId = e.dataTransfer.getData('element-id');
            if (source === 'dock' && elementId && elementId !== 'dock-home') document.getElementById(elementId)?.remove();
        });
    }
});