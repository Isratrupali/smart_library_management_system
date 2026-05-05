
let currentBookId = null;


async function checkAuth(role) {
    try {
        const resp = await fetch('/api/me'); // Ask server for current user info
        if (!resp.ok) { window.location.href = '/auth.html'; return; }
        const data = await resp.json();
        if (!data.loggedIn || data.role !== role) {
            window.location.href = '/auth.html'; // Wrong role: redirect to login
            return;
        }
        // Update UI with user name if element exists
        const nameEl = document.getElementById('user-name');
        if (nameEl) nameEl.textContent = data.name;
    } catch (e) {
        window.location.href = '/auth.html'; // Error: redirect to login
    }
}


async function logout() {
    await fetch('/api/logout', { method: 'POST' }); // Clear server session
    localStorage.removeItem('slms_user');            // Clear local storage
    window.location.href = '/auth.html';             // Go to login page
}


function showToast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    if (!container) return;
    const toast = document.createElement('div');
    toast.className = `toast-msg toast-${type}`; // Apply CSS class for styling
    toast.textContent = message;
    container.appendChild(toast);
    // Auto-remove after 3.5 seconds
    setTimeout(() => toast.remove(), 3500);
}


async function loadDashboard() {
    try {
        // Fetch user info for stats
        const meResp = await fetch('/api/me');
        const me = await meResp.json();

        // Update membership and fine stats
        const el = (id) => document.getElementById(id);
        if (el('stat-fine'))       el('stat-fine').textContent = me.fineBalance + ' BDT';
        if (el('stat-membership')) el('stat-membership').textContent = me.membership.toUpperCase();

        // Fetch active borrows
        const bResp = await fetch('/api/student/borrows');
        const bData = await bResp.json();
        const activeBorrows = (bData.borrows || []).filter(b => b.status !== 'returned');

        if (el('stat-borrows')) el('stat-borrows').textContent = activeBorrows.length;

        // Render borrows list
        const list = el('borrows-list');
        const loading = el('borrows-loading');
        if (loading) loading.style.display = 'none';
        if (list) {
            if (activeBorrows.length === 0) {
                list.innerHTML = `<div class="text-center py-4 text-muted-custom">
                    <div style="font-size:2.5rem;">📚</div>
                    <p class="mt-2">No active borrows. <a href="/student/catalog.html" class="text-cyan">Browse books →</a></p>
                </div>`;
            } else {
                list.innerHTML = activeBorrows.map(b => `
                    <div style="background:#ffffff;border:1px solid ${b.status==='overdue'?'rgba(220,38,38,0.35)':'rgba(37,99,235,0.2)'};border-radius:10px;padding:12px;margin-bottom:8px;display:flex;justify-content:space-between;align-items:center;box-shadow:0 1px 3px rgba(15,23,42,0.06);">
                      <div>
                        <div style="font-weight:600;">${b.title}</div>
                        <div class="text-muted-custom" style="font-size:0.78rem;">Due: ${b.dueDate}</div>
                        ${b.fine > 0 ? `<div class="text-amber" style="font-size:0.75rem;">Fine: ${b.fine} BDT</div>` : ''}
                      </div>
                      <div class="d-flex align-items-center gap-2">
                        <span class="badge-${b.status}">${b.status.toUpperCase()}</span>
                        <button onclick="returnBook(${b.id})" class="btn-sm-action btn-sm-green">Return</button>
                      </div>
                    </div>`).join('');
            }
        }

        // Fetch notifications
        const nResp = await fetch('/api/student/notifications');
        const nData = await nResp.json();
        const notifs = nData.notifications || [];
        if (el('stat-notifs')) el('stat-notifs').textContent = notifs.length;
        try {
            const billResp = await fetch('/api/student/billing');
            if (billResp.ok) {
                const billData = await billResp.json();
                const pending = billData.pendingTotal || 0;
                const dueEl = el('stat-billing-due');
                if (dueEl) {
                    dueEl.textContent = pending > 0 ? pending + ' BDT due' : '—';
                    dueEl.className = pending > 0 ? 'stat-value text-amber' : 'stat-value text-muted-custom';
                }
            }
        } catch (_) { /* optional */ }
        const nList = el('notifs-list');
        if (nList) {
            if (notifs.length === 0) {
                nList.innerHTML = '<p class="text-muted-custom">No notifications.</p>';
            } else {
                nList.innerHTML = notifs.map(n => `
                    <div style="padding:10px;border-bottom:1px solid rgba(15,23,42,0.08);${!n.isRead?'font-weight:600;':''}" >
                      <div style="font-size:0.875rem;">${n.message}</div>
                      <div class="text-muted-custom" style="font-size:0.72rem;margin-top:4px;">${n.createdAt}</div>
                    </div>`).join('');
            }
        }
    } catch (e) {
        showToast('Failed to load dashboard data.', 'error');
    }
}


async function searchBooks() {
    const keyword   = document.getElementById('search-keyword')?.value  || '';
    const genre     = document.getElementById('search-genre')?.value    || '';
    const type      = document.getElementById('search-type')?.value     || '';
    const sort      = document.getElementById('search-sort')?.value     || 'title';
    const available = document.getElementById('search-available')?.checked ? '1' : '';

    // Build query string from selected filters
    const params = new URLSearchParams({ keyword, genre, type, sort, available });
    const grid = document.getElementById('books-grid');
    if (grid) grid.innerHTML = '<div class="col-12 text-center py-4"><div class="spinner"></div></div>';

    try {
        const resp = await fetch(`/api/books?${params}`); // Call the books API with filters
        const data = await resp.json();
        const books = data.books || [];

        // Update results count
        const countEl = document.getElementById('results-count');
        if (countEl) countEl.textContent = `${books.length} book(s) found`;

        if (!grid) return;
        if (books.length === 0) {
            grid.innerHTML = '<div class="col-12 text-center py-5 text-muted-custom">No books match your search.</div>';
            return;
        }

        // Render book cards in a responsive grid
        grid.innerHTML = books.map(b => `
            <div class="col-md-3 col-sm-6">
              <div class="glass-card book-card p-3" onclick="openBookModal(${b.id})">
                <div class="d-flex justify-content-between align-items-start mb-2">
                  <span class="book-card-type ${b.bookType==='ebook'?'badge-ebook':b.bookType==='audio'?'badge-audio':'badge-active'}">
                    ${b.bookType==='ebook'?'💻 E-book':b.bookType==='audio'?'🎧 Audio':'📖 Physical'}
                  </span>
                </div>
                <div style="font-weight:600;font-size:0.875rem;margin-bottom:4px;">${b.title}</div>
                <div class="text-muted-custom" style="font-size:0.78rem;">${b.author}</div>
                <div class="text-muted-custom" style="font-size:0.72rem;">${b.genre} · ${b.publishedYear}</div>
                <div class="mt-2" style="font-size:0.75rem;color:${b.available?'#15803d':'#dc2626'};">
                  ${b.available ? '✓ Available ('+b.availableCopies+')' : '✗ Not Available'}
                </div>
              </div>
            </div>`).join('');
    } catch (e) {
        if (grid) grid.innerHTML = '<div class="col-12 text-center text-red">Failed to load books.</div>';
    }
}

// E-book / audio parts (multi-file) + legacy single URL
function studentEbookParts(b) {
    if (b.ebookUrls && Array.isArray(b.ebookUrls) && b.ebookUrls.length) {
        return b.ebookUrls.filter(x => x && x.url);
    }
    if (b.ebookUrl) return [{ url: b.ebookUrl, label: 'PDF' }];
    return [];
}
function studentAudioParts(b) {
    if (b.audioUrls && Array.isArray(b.audioUrls) && b.audioUrls.length) {
        return b.audioUrls.filter(x => x && x.url);
    }
    if (b.audioUrl) return [{ url: b.audioUrl, label: 'Listen' }];
    return [];
}


async function openBookModal(bookId) {
    currentBookId = bookId; // Store for use by borrowBook() / joinWaitlist()
    const modal = document.getElementById('book-modal');
    if (!modal) return;
    modal.classList.add('active'); // Show modal overlay

    try {
        const resp = await fetch(`/api/books/${bookId}`);
        const b = await resp.json();
        const eParts = studentEbookParts(b);
        const aParts = studentAudioParts(b);
        let digitalBlock = '';
        if (b.bookType === 'ebook') {
            digitalBlock = eParts.length
                ? `<div class="mt-3"><div class="text-muted-custom small mb-1">Read</div><ul class="list-unstyled mb-0 ps-0">${eParts.map((p, i) =>
                    `<li class="mb-2"><a href="${p.url}" target="_blank" rel="noopener" class="btn-primary-gradient" style="text-decoration:none;font-size:0.82rem;border:none;display:inline-block;">📄 ${p.label || ('Part ' + (i + 1))}</a></li>`).join('')}</ul></div>`
                : '<div class="mt-2 text-muted-custom small">No PDF file linked yet.</div>';
        } else if (b.bookType === 'audio') {
            digitalBlock = aParts.length
                ? `<div class="mt-3"><div class="text-muted-custom small mb-1">Listen</div>${aParts.map((p, i) =>
                    `<div class="mb-3"><div style="font-size:0.75rem;font-weight:600;">${p.label || ('Track ' + (i + 1))}</div><audio controls preload="metadata" style="width:100%;max-width:100%;"><source src="${p.url}"></audio></div>`).join('')}</div>`
                : '<div class="mt-2 text-muted-custom small">No audio file linked yet.</div>';
        }
        document.getElementById('modal-content').innerHTML = `
            <h5 class="font-display fw-800 mb-1">${b.title}</h5>
            <div class="text-muted-custom mb-3" style="font-size:0.85rem;">by ${b.author} · ${b.publishedYear}</div>
            <div class="d-flex gap-2 flex-wrap mb-3">
              <span class="badge-active">${b.genre}</span>
              <span class="${b.bookType==='ebook'?'badge-ebook':b.bookType==='audio'?'badge-audio':'badge-returned'}">${b.bookType}</span>
              <span style="font-size:0.72rem;color:${b.available?'#15803d':'#dc2626'};">${b.available ? '✓ '+b.availableCopies+' available' : '✗ None available'}</span>
            </div>
            <p class="text-muted-custom" style="font-size:0.875rem;">${b.description || 'No description available.'}</p>
            <div class="text-muted-custom" style="font-size:0.78rem;">ISBN: ${b.isbn} · Condition: ${b.bookCondition}</div>
            ${digitalBlock}`;

        // Show the right action button based on availability
        const borrowBtn   = document.getElementById('modal-borrow-btn');
        const waitlistBtn = document.getElementById('modal-waitlist-btn');
        const isDigital = b.bookType === 'ebook' || b.bookType === 'audio';
        if (borrowBtn)   borrowBtn.style.display   = (!isDigital && b.available) ? 'inline-block' : 'none';
        if (waitlistBtn) waitlistBtn.style.display = (!isDigital && !b.available) ? 'inline-block' : 'none';
    } catch (e) {
        document.getElementById('modal-content').textContent = 'Failed to load book details.';
    }
}

// Close modal and reset its content
function closeModal() {
    const modal = document.getElementById('book-modal');
    if (modal) modal.classList.remove('active'); // Hide the modal overlay
    currentBookId = null; // Clear the selected book
    const msg = document.getElementById('modal-msg');
    if (msg) msg.style.display = 'none';
}


async function borrowBook() {
    if (!currentBookId) return;
    try {
        const resp = await fetch('/api/student/borrow', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ bookId: currentBookId }) // Send book ID
        });
        const data = await resp.json();
        const msg = document.getElementById('modal-msg');
        if (msg) {
            msg.style.display = 'block';
            msg.style.cssText = `display:block;padding:0.75rem;border-radius:8px;font-size:0.875rem;background:${data.success?'#dcfce7':'#fef2f2'};color:${data.success?'#166534':'#991b1b'};border:1px solid ${data.success?'rgba(22,163,74,0.4)':'rgba(220,38,38,0.35)'};`;
            msg.textContent = data.message;
        }
        if (data.success) {
            showToast('Book borrowed successfully!', 'success');
            setTimeout(closeModal, 2000); // Close modal after 2s
        }
    } catch (e) {
        showToast('Failed to borrow book.', 'error');
    }
}


async function returnBook(transactionId) {
    if (!confirm('Confirm return this book?')) return; // Ask user to confirm
    try {
        const resp = await fetch('/api/student/return', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ transactionId }) // Send transaction ID
        });
        const data = await resp.json();
        if (data.success) {
            showToast(data.message, 'success');
            loadDashboard(); // Reload dashboard to show updated borrow list
        } else {
            showToast(data.message || 'Return failed.', 'error');
        }
    } catch (e) {
        showToast('Failed to return book.', 'error');
    }
}


async function joinWaitlist() {
    if (!currentBookId) return;
    try {
        const resp = await fetch('/api/student/waitlist', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ bookId: currentBookId })
        });
        const data = await resp.json();
        showToast(data.message, data.success ? 'success' : 'error');
        if (data.success) setTimeout(closeModal, 2000);
    } catch (e) {
        showToast('Failed to join waitlist.', 'error');
    }
}


async function loadDigital(type) {
    const grid = document.getElementById('digital-grid');
    if (grid) grid.innerHTML = '<div class="col-12 text-center py-4"><div class="spinner"></div></div>';

    // Update tab button styles
    const ebookBtn = document.getElementById('btn-ebook');
    const audioBtn = document.getElementById('btn-audio');
    if (ebookBtn && audioBtn) {
        if (type === 'ebook') {
            ebookBtn.className = 'btn-primary-gradient'; ebookBtn.style.border = 'none';
            audioBtn.className = 'btn-outline-glass';
        } else {
            audioBtn.className = 'btn-primary-gradient'; audioBtn.style.border = 'none';
            ebookBtn.className = 'btn-outline-glass';
        }
    }

    try {
        const resp = await fetch(`/api/books?type=${type}&sort=title`);
        const data = await resp.json();
        const books = data.books || [];
        if (!grid) return;
        if (books.length === 0) {
            grid.innerHTML = `<div class="col-12 text-center py-5 text-muted-custom">No ${type} books found.</div>`;
            return;
        }
        const icon = type === 'ebook' ? '💻' : '🎧';
        grid.innerHTML = books.map(b => {
            const eParts = studentEbookParts(b);
            const aParts = studentAudioParts(b);
            let mediaBlock = '';
            if (type === 'ebook') {
                mediaBlock = eParts.length
                    ? `<div class="d-flex flex-column gap-1 align-items-center">${eParts.map((p, i) =>
                        `<a href="${p.url}" target="_blank" rel="noopener" class="btn-sm-action btn-sm-cyan" style="text-decoration:none;">📄 ${p.label || ('Part ' + (i + 1))}</a>`).join('')}</div>`
                    : `<span class="text-muted-custom" style="font-size:0.78rem;">No PDF yet</span>`;
            } else {
                mediaBlock = aParts.length
                    ? `<div class="d-flex flex-column gap-2 align-items-center w-100">${aParts.map((p, i) =>
                        `<div class="w-100" style="max-width:260px;"><div class="text-muted-custom" style="font-size:0.7rem;">${p.label || ('Track ' + (i + 1))}</div><audio controls preload="metadata" style="width:100%;"><source src="${p.url}"></audio></div>`).join('')}</div>`
                    : `<span class="text-muted-custom" style="font-size:0.78rem;">No audio yet</span>`;
            }
            return `
            <div class="col-md-3 col-sm-6">
              <div class="glass-card p-4 text-center h-100" style="border-color:rgba(${type==='ebook'?'37,99,235':'234,88,12'},0.3);">
                <div style="font-size:2.5rem;margin-bottom:0.75rem;">${icon}</div>
                <div style="font-weight:600;font-size:0.875rem;margin-bottom:4px;">${b.title}</div>
                <div class="text-muted-custom" style="font-size:0.78rem;">${b.author}</div>
                <div class="text-muted-custom" style="font-size:0.72rem;">${b.genre}</div>
                <div class="mt-3 d-flex flex-column align-items-center gap-2">${mediaBlock}</div>
              </div>
            </div>`;
        }).join('');
    } catch (e) {
        showToast('Failed to load digital books.', 'error');
    }
}


async function loadSeats() {
    const date = document.getElementById('seat-date')?.value || '';
    try {
        const resp = await fetch(`/api/student/seats?date=${date}`);
        const data = await resp.json();
        const booked = data.booked || [];
        const myBookings = data.myBookings || [];

        const room = document.getElementById('seat-room')?.value || '';
        const taken = new Set(booked.map(b => `${b.seatNumber}-${b.timeSlot}-${b.room}`));
        const slot = document.getElementById('seat-timeslot')?.value;

        // Render seat grid: A1-A5, B1-B5
        const grid = document.getElementById('seat-grid');
        if (grid) {
            const seats = ['A1','A2','A3','A4','A5','B1','B2','B3','B4','B5'];
            grid.innerHTML = seats.map(s => {
                const isTaken = taken.has(`${s}-${slot}-${room}`);
                return `<button onclick="selectSeat('${s}')" id="seat-btn-${s}"
                    style="width:48px;height:48px;border-radius:8px;border:1px solid ${isTaken?'rgba(220,38,38,0.45)':'rgba(22,163,74,0.45)'};
                    background:${isTaken?'#fef2f2':'#dcfce7'};
                    color:${isTaken?'#dc2626':'#15803d'};font-size:0.75rem;font-weight:700;cursor:${isTaken?'not-allowed':'pointer'};"
                    ${isTaken ? 'disabled' : ''}>${s}</button>`;
            }).join('');
        }

        // Render my bookings list
        const myList = document.getElementById('my-bookings-list');
        if (myList) {
            if (myBookings.length === 0) {
                myList.innerHTML = '<p class="text-muted-custom">No bookings yet.</p>';
            } else {
                myList.innerHTML = myBookings.map(b => `
                    <div style="padding:12px;border-bottom:1px solid rgba(15,23,42,0.08);">
                      <div style="font-weight:600;">Seat ${b.seatNumber} — ${b.room}</div>
                      <div class="text-muted-custom" style="font-size:0.78rem;">${b.bookingDate} · ${b.timeSlot}</div>
                      <span class="badge-${b.status}">${b.status}</span>
                    </div>`).join('');
            }
        }
    } catch (e) {
        showToast('Failed to load seat data.', 'error');
    }
}

// selectSeat: Visually select a seat and store its number
function selectSeat(seatNumber) {
    document.getElementById('selected-seat').value = seatNumber; // Store chosen seat
    showToast(`Seat ${seatNumber} selected`, 'info');
    // Highlight the selected seat button
    document.querySelectorAll('[id^="seat-btn-"]').forEach(btn => {
        btn.style.boxShadow = '';
    });
    const selectedBtn = document.getElementById(`seat-btn-${seatNumber}`);
    if (selectedBtn) selectedBtn.style.boxShadow = '0 0 0 3px rgba(37,99,235,0.5)';
}

// bookSeat: POST /api/student/seats/book with the selected seat info
async function bookSeat() {
    const seat     = document.getElementById('selected-seat').value;
    const date     = document.getElementById('seat-date').value;
    const timeSlot = document.getElementById('seat-timeslot').value;
    const room     = document.getElementById('seat-room').value;

    if (!seat) { showToast('Please select a seat first.', 'error'); return; }
    if (!date) { showToast('Please select a date.', 'error'); return; }

    try {
        const resp = await fetch('/api/student/seats/book', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ seatNumber: seat, date, timeSlot, room })
        });
        const data = await resp.json();
        const msgEl = document.getElementById('seat-msg');
        if (msgEl) {
            msgEl.style.display = 'block';
            msgEl.style.cssText = `display:block;padding:0.75rem;border-radius:8px;font-size:0.875rem;background:${data.success?'#dcfce7':'#fef2f2'};color:${data.success?'#166534':'#991b1b'};border:1px solid ${data.success?'rgba(22,163,74,0.4)':'rgba(220,38,38,0.35)'};`;
            msgEl.textContent = data.message;
        }
        if (data.success) {
            showToast('Seat booked!', 'success');
            loadSeats(); // Refresh seat grid
        }
    } catch (e) {
        showToast('Booking failed.', 'error');
    }
}


async function loadMembership() {
    try {
        const resp = await fetch('/api/me');
        const me = await resp.json();
        const plan = me.membership || 'basic';

        const planEl = document.getElementById('current-plan');
        if (planEl) planEl.textContent = plan.charAt(0).toUpperCase() + plan.slice(1);

        // Show "Current Plan" badge on the active tier, and upgrade buttons on others
        const tiers = ['basic', 'premium', 'gold'];
        const rank = (t) => ({ basic: 0, premium: 1, gold: 2 }[t] ?? -1);
        const curRank = rank(plan);
        tiers.forEach(tier => {
            const btnDiv = document.getElementById(`${tier}-btn`);
            if (!btnDiv) return;
            if (tier === plan) {
                btnDiv.innerHTML = `<span class="badge-active">✓ Current Plan</span>`;
            } else if (rank(tier) <= curRank) {
                btnDiv.innerHTML = `<span class="text-muted-custom" style="font-size:0.8rem;">Lower or same tier</span>`;
            } else if (tier === 'basic') {
                btnDiv.innerHTML = `<span class="text-muted-custom" style="font-size:0.8rem;">Free tier</span>`;
            } else {
                btnDiv.innerHTML = `<button type="button" onclick="requestMembershipPurchase('${tier}')" class="btn-primary-gradient" style="border:none;font-size:0.85rem;">Request ${tier.charAt(0).toUpperCase()+tier.slice(1)} (creates invoice)</button>`;
            }
        });
    } catch (e) {
        showToast('Failed to load membership.', 'error');
    }
}

// requestMembershipPurchase: POST /api/student/membership/purchase
async function requestMembershipPurchase(tier) {
    const msgEl = document.getElementById('upgrade-msg') || document.getElementById('billing-quick-msg');
    try {
        const resp = await fetch('/api/student/membership/purchase', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ tier })
        });
        const data = await resp.json();
        if (msgEl) {
            msgEl.style.display = 'block';
            if (resp.ok && data.success) {
                msgEl.style.cssText = 'display:block;padding:0.75rem;border-radius:8px;background:#dcfce7;color:#166534;border:1px solid rgba(22,163,74,0.4);';
                msgEl.textContent = data.message || 'Invoice created.';
                showToast('Invoice created. Check Billing & notifications.', 'success');
                loadMembership();
                if (typeof loadStudentBillingPage === 'function') loadStudentBillingPage();
            } else {
                msgEl.style.cssText = 'display:block;padding:0.75rem;border-radius:8px;background:#fef2f2;color:#991b1b;border:1px solid rgba(220,38,38,0.35);';
                msgEl.textContent = data.error || data.message || 'Request failed.';
            }
        }
    } catch (e) {
        if (msgEl) {
            msgEl.style.display = 'block';
            msgEl.style.cssText = 'display:block;padding:0.75rem;border-radius:8px;background:#fef2f2;color:#991b1b;border:1px solid rgba(220,38,38,0.35);';
            msgEl.textContent = 'Could not reach server.';
        }
    }
}

// ---- Billing dashboard (billing.html) helpers ----
function billingTierRank(t) {
    return ({ basic: 0, premium: 1, gold: 2 }[String(t).toLowerCase()] ?? -1);
}

function billingFormatTaka(n) {
    return '৳ ' + Number(n || 0).toLocaleString(undefined, { maximumFractionDigits: 0 });
}

function exportBillingCsv() {
    const rows = window.__billingBills || [];
    if (!rows.length) {
        showToast('No rows to export.', 'info');
        return;
    }
    const headers = ['id', 'targetTier', 'amountBdt', 'status', 'createdAt', 'paidAt', 'description'];
    const lines = [headers.join(',')];
    rows.forEach(b => {
        lines.push(headers.map(h => {
            let v = b[h] ?? '';
            v = String(v).replace(/"/g, '""');
            if (/[",\n]/.test(v)) v = '"' + v + '"';
            return v;
        }).join(','));
    });
    const blob = new Blob([lines.join('\n')], { type: 'text/csv;charset=utf-8;' });
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = 'smartlibrary-billing.csv';
    a.click();
    URL.revokeObjectURL(a.href);
    showToast('CSV downloaded.', 'success');
}

function focusBillingQuickPay() {
    document.getElementById('quick-pay-card')?.scrollIntoView({ behavior: 'smooth', block: 'center' });
    setTimeout(() => document.getElementById('billing-quick-select')?.focus(), 400);
}

function onBillingQuickSelectChange() {
    const sel = document.getElementById('billing-quick-select');
    const amt = document.getElementById('billing-quick-amount');
    if (!sel || !amt) return;
    const opt = sel.options[sel.selectedIndex];
    if (opt && opt.dataset.amount) amt.value = opt.dataset.amount;
    else amt.value = '';
}

function rebuildBillingQuickSelect(bills, plans, currentTier) {
    const sel = document.getElementById('billing-quick-select');
    if (!sel) return;
    const pending = bills.filter(b => b.status === 'pending');
    const cur = String(currentTier || 'basic').toLowerCase();
    sel.innerHTML = '';

    pending.forEach(b => {
        const o = document.createElement('option');
        o.value = 'bill:' + b.id;
        o.textContent = 'Invoice #' + b.id + ' — ' + b.targetTier + ' — ' + billingFormatTaka(b.amountBdt);
        o.dataset.amount = String(b.amountBdt);
        sel.appendChild(o);
    });

    (plans || []).forEach(p => {
        const tn = String(p.tierName).toLowerCase();
        if (billingTierRank(tn) <= billingTierRank(cur)) return;
        if (Number(p.monthlyFee) <= 0) return;
        const exists = bills.some(x => x.status === 'pending' && String(x.targetTier).toLowerCase() === tn);
        if (exists) return;
        const o = document.createElement('option');
        o.value = 'tier:' + tn;
        o.textContent = 'New: ' + tn.charAt(0).toUpperCase() + tn.slice(1) + ' — ' + billingFormatTaka(p.monthlyFee) + '/mo';
        o.dataset.amount = String(p.monthlyFee);
        sel.appendChild(o);
    });

    if (sel.options.length === 0) {
        const o = document.createElement('option');
        o.value = '';
        o.textContent = 'No pending invoices — pick a plan below or visit Membership';
        sel.appendChild(o);
    }
    onBillingQuickSelectChange();
}

function billingQuickPaySubmit() {
    const sel = document.getElementById('billing-quick-select');
    const msg = document.getElementById('billing-quick-msg');
    if (!sel || !sel.value) {
        showToast('Select an invoice or membership option.', 'error');
        return;
    }
    const v = sel.value;
    if (v.startsWith('bill:')) {
        const id = v.slice(5);
        const amt = document.getElementById('billing-quick-amount')?.value || '0';
        const btn = document.querySelector('.billing-quick-pay-btn');
        
        if (msg) {
            msg.style.display = 'block';
            msg.style.cssText = 'display:block;padding:0.75rem;border-radius:10px;background:#e8f0fe;color:#174ea6;border:1px solid rgba(26,115,232,0.35);';
            msg.innerHTML = '<div class="spinner" style="width:16px;height:16px;display:inline-block;vertical-align:middle;margin-right:8px;border-color:#174ea6;border-top-color:transparent;"></div> Processing payment...';
        }
        if (btn) btn.disabled = true;

        fetch('/api/student/billing/pay', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ billId: parseInt(id) })
        }).then(res => res.json().then(data => ({ status: res.status, ok: res.ok, data })))
        .then(({ status, ok, data }) => {
            if (ok) {
                if (msg) {
                    msg.style.background = '#e6f4ea';
                    msg.style.color = '#137333';
                    msg.style.borderColor = 'rgba(19,115,51,0.35)';
                    msg.innerHTML = '✅ Payment successful! Total paid and amount due updated.';
                }
                showToast('Payment successful!', 'success');
                setTimeout(() => {
                    if (typeof loadStudentBillingPage === 'function') loadStudentBillingPage();
                    if (btn) btn.disabled = false;
                }, 1500);
            } else {
                if (msg) {
                    msg.style.background = '#fce8e6';
                    msg.style.color = '#c5221f';
                    msg.style.borderColor = 'rgba(197,34,31,0.35)';
                    msg.innerHTML = '❌ ' + (data.error || 'Payment failed');
                }
                if (btn) btn.disabled = false;
            }
        }).catch(err => {
            if (msg) {
                msg.style.background = '#fce8e6';
                msg.style.color = '#c5221f';
                msg.innerHTML = '❌ Network error.';
            }
            if (btn) btn.disabled = false;
        });

        return;
    }
    if (v.startsWith('tier:')) {
        requestMembershipPurchase(v.slice(5));
        return;
    }
}

function renderBillingChart(year, bills) {
    const area = document.getElementById('billing-chart-area');
    if (!area) return;
    const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
    const totals = Array(12).fill(0);
    (bills || []).forEach(b => {
        if (b.status !== 'paid') return;
        const raw = b.paidAt || b.createdAt || '';
        if (!raw || raw.length < 7) return;
        const y = parseInt(raw.slice(0, 4), 10);
        const m = parseInt(raw.slice(5, 7), 10) - 1;
        if (y !== year || m < 0 || m > 11) return;
        totals[m] += Number(b.amountBdt) || 0;
    });
    const max = Math.max(...totals, 1);
    const hasData = totals.some(t => t > 0);
    if (!hasData) {
        area.innerHTML = '<div class="billing-chart-empty"><div class="billing-chart-empty-icon">📉</div><div>No data yet. Complete a payment at the library to see your history.</div></div>';
        return;
    }
    const barMax = 140;
    area.innerHTML = '<div class="billing-chart-body">' + totals.map((t, i) => {
        const hPx = t <= 0 ? 4 : Math.max(4, Math.round((t / max) * barMax));
        return '<div class="billing-chart-col"><div class="billing-chart-bar ' + (t <= 0 ? 'is-zero' : '') + '" style="height:' + hPx + 'px" title="' + billingFormatTaka(t) + '"></div><span class="billing-chart-month">' + months[i] + '</span></div>';
    }).join('') + '</div>';
}

function renderBillingPlans(plans, currentTier, pendingTiers) {
    const row = document.getElementById('billing-plans-row');
    if (!row) return;
    const cur = String(currentTier || 'basic').toLowerCase();
    const pend = new Set((pendingTiers || []).map(t => String(t).toLowerCase()));

    row.innerHTML = (plans || []).map(p => {
        const tn = String(p.tierName).toLowerCase();
        const rank = billingTierRank(tn);
        const curR = billingTierRank(cur);
        let btn = '';
        if (tn === cur) {
            btn = '<span class="badge-active">Current plan</span>';
        } else if (rank < curR) {
            btn = '<span class="text-muted-custom small">Lower tier</span>';
        } else if (pend.has(tn)) {
            btn = '<span class="badge-overdue">Invoice pending</span>';
        } else if (Number(p.monthlyFee) <= 0) {
            btn = '<span class="text-muted-custom small">No fee</span>';
        } else {
            btn = '<button type="button" class="billing-btn-primary-solid" onclick="requestMembershipPurchase(\'' + tn + '\')">Subscribe</button>';
        }
        return '<div class="col-md-4"><div class="billing-plan-card"><h6>' + (tn.charAt(0).toUpperCase() + tn.slice(1)) + '</h6>' +
            '<div class="billing-plan-price">' + billingFormatTaka(p.monthlyFee) + '<span class="text-muted-custom" style="font-size:0.85rem;font-weight:600;"> /month</span></div>' +
            '<ul><li>Borrow up to <strong>' + p.maxBooks + '</strong> books</li><li><strong>' + p.borrowDays + '</strong> days per loan</li><li>Fine: ' + billingFormatTaka(p.finePerDay) + ' / day overdue</li></ul>' +
            btn + '</div></div>';
    }).join('');
}

function filterBillingTable() {
    const body = document.getElementById('billing-body');
    const wrap = document.getElementById('billing-table-wrap');
    const empty = document.getElementById('billing-empty');
    const loading = document.getElementById('billing-loading');
    const countEl = document.getElementById('billing-record-count');
    if (!body) return;
    if (loading) loading.style.display = 'none';

    const all = window.__billingBills || [];
    const q = (document.getElementById('billing-txn-search')?.value || '').toLowerCase().trim();
    const st = document.getElementById('billing-txn-status')?.value || 'all';

    const rows = all.filter(b => {
        if (st !== 'all' && b.status !== st) return false;
        if (!q) return true;
        const hay = (String(b.id) + ' ' + b.targetTier + ' ' + b.status + ' ' + (b.description || '')).toLowerCase();
        return hay.includes(q);
    });

    if (all.length === 0) {
        if (wrap) wrap.style.display = 'none';
        if (empty) {
            empty.style.display = 'block';
            empty.innerHTML = '<div class="icon">🧾</div><div>No transactions found</div><p class="small mt-2 mb-0">Request a membership upgrade to create your first invoice.</p>';
        }
        if (countEl) countEl.textContent = '0 records';
        return;
    }

    if (rows.length === 0) {
        if (wrap) wrap.style.display = 'none';
        if (empty) {
            empty.style.display = 'block';
            empty.innerHTML = '<div class="icon">🔍</div><div>No matches</div><p class="small mt-2 mb-0">Try another search or status filter.</p>';
        }
        if (countEl) countEl.textContent = '0 records (filtered)';
        return;
    }

    if (empty) empty.style.display = 'none';
    if (wrap) wrap.style.display = 'block';
    body.innerHTML = rows.map(b => `
            <tr>
              <td>#${b.id}</td>
              <td>${b.targetTier}</td>
              <td>${billingFormatTaka(b.amountBdt)}</td>
              <td>${b.status === 'pending' ? '<span class="badge-overdue">DUE</span>' : '<span class="badge-active">PAID</span>'}</td>
              <td style="font-size:0.78rem;color:#64748b;">${b.createdAt || '—'}</td>
              <td style="font-size:0.78rem;color:#64748b;">${b.paidAt || '—'}</td>
            </tr>`).join('');
    if (countEl) countEl.textContent = rows.length + ' record' + (rows.length !== 1 ? 's' : '');
}

// loadStudentBillingPage: billing dashboard + legacy stats hook
async function loadStudentBillingPage() {
    const loading = document.getElementById('billing-loading');
    const wrap = document.getElementById('billing-table-wrap');
    const body = document.getElementById('billing-body');
    const empty = document.getElementById('billing-empty');
    const summary = document.getElementById('billing-summary');
    const pendingEl = document.getElementById('billing-pending-total');

    const isFullDashboard = !!document.getElementById('billing-stat-paid');
    if (!body && !isFullDashboard) return;

    if (loading) loading.style.display = 'block';
    if (wrap) wrap.style.display = 'none';
    if (empty) empty.style.display = 'none';

    try {
        const [billResp, meResp, planResp] = await Promise.all([
            fetch('/api/student/billing'),
            fetch('/api/me'),
            fetch('/api/student/membership-plans')
        ]);
        const data = await billResp.json();
        const me = meResp.ok ? await meResp.json() : {};
        const planData = planResp.ok ? await planResp.json() : { plans: [] };

        const bills = data.bills || [];
        window.__billingBills = bills;

        const pendingTotal = data.pendingTotal || 0;
        let totalPaid = 0;
        bills.forEach(b => { if (b.status === 'paid') totalPaid += Number(b.amountBdt) || 0; });

        if (summary && pendingEl) {
            if (pendingTotal > 0) {
                summary.style.display = 'block';
                pendingEl.textContent = pendingTotal + ' BDT';
            } else {
                summary.style.display = 'none';
            }
        }

        if (isFullDashboard) {
            const elPaid = document.getElementById('billing-stat-paid');
            const elDue = document.getElementById('billing-stat-due');
            const elCnt = document.getElementById('billing-stat-count');
            const elNext = document.getElementById('billing-stat-next');
            if (elPaid) elPaid.textContent = billingFormatTaka(totalPaid);
            if (elDue) elDue.textContent = billingFormatTaka(pendingTotal);
            if (elCnt) elCnt.textContent = String(bills.length);

            const pendingBills = bills.filter(b => b.status === 'pending');
            if (elNext) {
                if (pendingBills.length === 0) elNext.textContent = '—';
                else {
                    const sorted = [...pendingBills].sort((a, b) => (a.createdAt || '').localeCompare(b.createdAt || ''));
                    const d = sorted[0].createdAt || '';
                    elNext.textContent = d ? d.split(' ')[0] : '—';
                }
            }

            const wName = document.getElementById('billing-wallet-name');
            const wPaid = document.getElementById('billing-wallet-paidline');
            const mem = me.membership || 'basic';
            const memLabel = mem.charAt(0).toUpperCase() + mem.slice(1);
            if (wName) wName.textContent = me.name || 'Student';
            if (wPaid) wPaid.textContent = billingFormatTaka(totalPaid) + ' total paid · ' + memLabel + ' plan';

            const yearSel = document.getElementById('billing-chart-year');
            if (yearSel && yearSel.options.length === 0) {
                const y = new Date().getFullYear();
                for (let i = 0; i < 5; i++) {
                    const opt = document.createElement('option');
                    opt.value = String(y - i);
                    opt.textContent = String(y - i);
                    yearSel.appendChild(opt);
                }
            }
            const chartYear = parseInt(yearSel?.value || String(new Date().getFullYear()), 10);
            renderBillingChart(chartYear, bills);

            rebuildBillingQuickSelect(bills, planData.plans || [], me.membership || 'basic');
            renderBillingPlans(planData.plans || [], me.membership || 'basic', pendingBills.map(b => b.targetTier));
        }

        if (loading) loading.style.display = 'none';
        filterBillingTable();
    } catch (e) {
        if (loading) loading.style.display = 'none';
        showToast('Failed to load billing.', 'error');
    }
}
