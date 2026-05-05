

// Multi-part PDF / audio for one catalog entry (manage-books)
let libEbookParts = [];
let libAudioParts = [];

function renderLibMediaLists() {
    const eu = document.getElementById('ebook-parts-ul');
    const au = document.getElementById('audio-parts-ul');
    if (eu) {
        eu.innerHTML = libEbookParts.map((p, i) => `
            <li class="d-flex justify-content-between align-items-start gap-2 py-1 border-bottom" style="border-color:rgba(15,23,42,0.08)!important;">
              <span style="word-break:break-all;"><strong>${escapeHtml(p.label)}</strong><br><a href="${escapeHtml(p.url)}" target="_blank" rel="noopener" class="text-muted-custom">${escapeHtml(p.url)}</a></span>
              <button type="button" class="btn-sm-action btn-sm-red flex-shrink-0" onclick="removeLibMediaPart('ebook',${i})">✕</button>
            </li>`).join('') || '<li class="text-muted-custom">No PDFs yet — use Add PDF.</li>';
    }
    if (au) {
        au.innerHTML = libAudioParts.map((p, i) => `
            <li class="d-flex justify-content-between align-items-start gap-2 py-1 border-bottom" style="border-color:rgba(15,23,42,0.08)!important;">
              <span style="word-break:break-all;"><strong>${escapeHtml(p.label)}</strong><br><a href="${escapeHtml(p.url)}" target="_blank" rel="noopener" class="text-muted-custom">${escapeHtml(p.url)}</a></span>
              <button type="button" class="btn-sm-action btn-sm-red flex-shrink-0" onclick="removeLibMediaPart('audio',${i})">✕</button>
            </li>`).join('') || '<li class="text-muted-custom">No audio yet — use Add audio.</li>';
    }
}

function removeLibMediaPart(kind, i) {
    if (kind === 'ebook') libEbookParts.splice(i, 1);
    else libAudioParts.splice(i, 1);
    renderLibMediaLists();
    const editId = document.getElementById('b-edit-id')?.value;
    if (editId) saveBook({ silent: true });
}

function normalizePartsFromApi(book) {
    let e = [];
    if (book.ebookUrls && Array.isArray(book.ebookUrls) && book.ebookUrls.length) {
        e = book.ebookUrls.map(x => ({ url: x.url || '', label: String(x.label || 'Part') }));
    } else if (book.ebookUrl) {
        e = [{ url: book.ebookUrl, label: 'PDF' }];
    }
    let a = [];
    if (book.audioUrls && Array.isArray(book.audioUrls) && book.audioUrls.length) {
        a = book.audioUrls.map(x => ({ url: x.url || '', label: String(x.label || 'Track') }));
    } else if (book.audioUrl) {
        a = [{ url: book.audioUrl, label: 'Audio' }];
    }
    return { e, a };
}

async function openEditBookById(id) {
    try {
        const resp = await fetch('/api/books/' + id);
        if (!resp.ok) throw new Error('fail');
        const book = await resp.json();
        document.getElementById('b-edit-id').value = book.id;
        document.getElementById('modal-title').textContent = 'Edit Book';
        document.getElementById('b-title').value = book.title || '';
        document.getElementById('b-author').value = book.author || '';
        document.getElementById('b-isbn').value = book.isbn || '';
        document.getElementById('b-genre').value = book.genre || 'Computer Science';
        document.getElementById('b-type').value = book.bookType || 'physical';
        document.getElementById('b-copies').value = book.totalCopies ?? 1;
        document.getElementById('b-year').value = book.publishedYear ?? 2024;
        document.getElementById('b-dept').value = book.department || '';
        document.getElementById('b-desc').value = book.description || '';
        const norm = normalizePartsFromApi(book);
        libEbookParts = norm.e;
        libAudioParts = norm.a;
        const ef = document.getElementById('b-ebook-file');
        const af = document.getElementById('b-audio-file');
        if (ef) ef.value = '';
        if (af) af.value = '';
        renderLibMediaLists();
        syncMediaPanels();
        document.getElementById('book-edit-modal').classList.add('active');
    } catch (e) {
        showToast('Could not load book.', 'error');
    }
}


async function checkLibrarian() {
    try {
        const resp = await fetch('/api/me');
        if (!resp.ok) { window.location.href = '/auth.html'; return; }
        const data = await resp.json();
        if (!data.loggedIn || (data.role !== 'librarian' && data.role !== 'admin')) {
            window.location.href = '/auth.html'; // Wrong role: redirect
        }
    } catch (e) { window.location.href = '/auth.html'; }
}

// logout: End session and go to login page
async function logout() {
    await fetch('/api/logout', { method: 'POST' });
    window.location.href = '/auth.html';
}

// showToast: Brief popup notification
function showToast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    if (!container) return;
    const toast = document.createElement('div');
    toast.className = `toast-msg toast-${type}`;
    toast.textContent = message;
    container.appendChild(toast);
    setTimeout(() => toast.remove(), 3500); // Auto-remove after 3.5s
}


async function loadLibDashboard() {
    try {
        // Fetch librarian stats from GET /api/librarian/stats
        const resp = await fetch('/api/librarian/stats');
        const s = await resp.json();

        const el = (id) => document.getElementById(id);
        if (el('stat-books'))     el('stat-books').textContent     = s.totalBooks    || 0;
        if (el('stat-students'))  el('stat-students').textContent  = s.totalStudents || 0;
        if (el('stat-active'))    el('stat-active').textContent    = s.activeLoans   || 0;
        if (el('stat-overdue'))   el('stat-overdue').textContent   = s.overdueLoans  || 0;
        if (el('stat-fines'))     el('stat-fines').textContent     = s.totalFines    || 0;
        if (el('stat-requests'))  el('stat-requests').textContent  = s.pendingRequests || 0;
        if (el('stat-pending-bills')) el('stat-pending-bills').textContent = s.pendingBills ?? 0;

        // Load overdue books list from GET /api/librarian/overdue
        const oResp = await fetch('/api/librarian/overdue');
        const oData = await oResp.json();
        const overdueEl = el('overdue-list');
        if (overdueEl) {
            const items = oData.overdue || [];
            if (items.length === 0) {
                overdueEl.innerHTML = '<p class="text-green">✓ No overdue books!</p>';
            } else {
                overdueEl.innerHTML = items.map(o => `
                    <div style="padding:10px;border-bottom:1px solid rgba(15,23,42,0.08);">
                      <div style="font-weight:600;font-size:0.875rem;">${o.title}</div>
                      <div class="text-muted-custom" style="font-size:0.78rem;">${o.name} · Due: ${o.dueDate}</div>
                      <span class="text-red" style="font-size:0.75rem;">Fine: ${o.fine} BDT</span>
                    </div>`).join('');
            }
        }
    } catch (e) { showToast('Failed to load dashboard.', 'error'); }
}


async function loadLibBooks() {
    const keyword = document.getElementById('lib-search')?.value || '';
    const genre   = document.getElementById('lib-genre')?.value   || '';
    const loading = document.getElementById('lib-books-loading');
    const table   = document.getElementById('lib-books-table');
    const body    = document.getElementById('lib-books-body');

    if (loading) loading.style.display = 'block';
    if (table)   table.style.display   = 'none';

    try {
        const resp = await fetch(`/api/books?keyword=${encodeURIComponent(keyword)}&genre=${encodeURIComponent(genre)}&sort=title`);
        const data = await resp.json();
        const books = data.books || [];

        if (loading) loading.style.display = 'none';
        if (table)   table.style.display   = 'table';
        if (body) {
            body.innerHTML = books.map(b => `
                <tr>
                  <td style="font-weight:600;">${b.title}</td>
                  <td>${b.author}</td>
                  <td>${b.genre}</td>
                  <td>${b.bookType === 'ebook' ? '💻 E-book' : b.bookType === 'audio' ? '🎧 Audio' : '📖 Physical'}</td>
                  <td>${b.totalCopies}</td>
                  <td style="color:${b.available?'#15803d':'#dc2626'}">${b.availableCopies}</td>
                  <td>
                    <button onclick="openEditBookById(${b.id})" class="btn-sm-action btn-sm-cyan me-1">Edit</button>
                    <button onclick="deleteBook(${b.id},'${b.title.replace(/'/g,"\\'")}')" class="btn-sm-action btn-sm-red">Delete</button>
                  </td>
                </tr>`).join('');
        }
    } catch (e) {
        if (loading) loading.style.display = 'none';
        showToast('Failed to load books.', 'error');
    }
}

// Show PDF / audio upload rows based on book type
function syncMediaPanels() {
    const t = document.getElementById('b-type')?.value || 'physical';
    const pe = document.getElementById('ebook-upload-panel');
    const pa = document.getElementById('audio-upload-panel');
    if (pe) pe.style.display = t === 'ebook' ? 'block' : 'none';
    if (pa) pa.style.display = t === 'audio' ? 'block' : 'none';
    if (t === 'physical') {
        libEbookParts = [];
        libAudioParts = [];
        renderLibMediaLists();
    }
}

// POST multipart to /api/books/upload-media; stores URL in hidden field
async function uploadBookMedia(kind) {
    const inputId = kind === 'ebook' ? 'b-ebook-file' : 'b-audio-file';
    const input = document.getElementById(inputId);
    if (!input?.files?.length) {
        showToast('Choose a file first.', 'error');
        return;
    }
    const fd = new FormData();
    fd.append('file', input.files[0]);
    fd.append('category', kind);
    try {
        const resp = await fetch('/api/books/upload-media', { method: 'POST', body: fd });
        const data = await resp.json();
        if (!resp.ok || !data.success) {
            showToast(data.error || data.message || 'Upload failed', 'error');
            return;
        }
        const url = data.url || '';
        const labelIn = window.prompt(
            kind === 'ebook' ? 'Label for this PDF (e.g. Chapter 1) — Cancel to skip' : 'Label for this track — Cancel to skip',
            ''
        );
        const autoNum = kind === 'ebook' ? libEbookParts.length + 1 : libAudioParts.length + 1;
        const label = (labelIn && String(labelIn).trim()) ? String(labelIn).trim()
            : (kind === 'ebook' ? ('Part ' + autoNum) : ('Track ' + autoNum));
        const part = { url, label };
        if (kind === 'ebook') libEbookParts.push(part);
        else libAudioParts.push(part);
        renderLibMediaLists();
        input.value = '';
        showToast('File added.', 'success');
        const editId = document.getElementById('b-edit-id')?.value;
        if (editId) await saveBook({ silent: true });
    } catch (e) {
        showToast('Upload failed.', 'error');
    }
}

// Open Add Book modal (empty form)
function openAddBookModal() {
    document.getElementById('b-edit-id').value = ''; // No ID = new book
    document.getElementById('modal-title').textContent = 'Add New Book';
    ['b-title','b-author','b-isbn','b-dept','b-desc'].forEach(id => {
        const el = document.getElementById(id); if (el) el.value = '';
    });
    document.getElementById('b-copies').value = 1;
    document.getElementById('b-year').value   = 2024;
    document.getElementById('b-type').value   = 'physical';
    libEbookParts = [];
    libAudioParts = [];
    const ef = document.getElementById('b-ebook-file');
    const af = document.getElementById('b-audio-file');
    if (ef) ef.value = '';
    if (af) af.value = '';
    renderLibMediaLists();
    syncMediaPanels();
    document.getElementById('book-edit-modal').classList.add('active');
}

function closeBookModal() {
    document.getElementById('book-edit-modal').classList.remove('active');
    const msg = document.getElementById('book-form-msg');
    if (msg) msg.style.display = 'none';
}

// saveBook: POST (add) or PUT (edit) a book
async function saveBook(opts) {
    const silent = opts && opts.silent;
    const editId  = document.getElementById('b-edit-id').value;
    const bookType = document.getElementById('b-type').value;
    let ebookUrls = libEbookParts.map(p => ({ url: p.url, label: p.label }));
    let audioUrls = libAudioParts.map(p => ({ url: p.url, label: p.label }));
    if (bookType === 'physical') { ebookUrls = []; audioUrls = []; }
    else if (bookType === 'ebook') { audioUrls = []; }
    else if (bookType === 'audio') { ebookUrls = []; }

    const payload = {
        title:           document.getElementById('b-title').value,
        author:          document.getElementById('b-author').value,
        isbn:            document.getElementById('b-isbn').value,
        genre:           document.getElementById('b-genre').value,
        bookType,
        totalCopies:     parseInt(document.getElementById('b-copies').value),
        availableCopies: parseInt(document.getElementById('b-copies').value),
        publishedYear:   parseInt(document.getElementById('b-year').value),
        department:      document.getElementById('b-dept').value,
        description:     document.getElementById('b-desc').value,
        ebookUrls,
        audioUrls,
        ebookUrl: ebookUrls[0]?.url || '',
        audioUrl: audioUrls[0]?.url || '',
    };

    const url    = editId ? `/api/books/${editId}` : '/api/books';
    const method = editId ? 'PUT' : 'POST';

    try {
        const resp = await fetch(url, {
            method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        const data = await resp.json();
        const msgEl = document.getElementById('book-form-msg');
        if (!silent && msgEl) {
            msgEl.style.cssText = `display:block;padding:0.75rem;border-radius:8px;font-size:0.875rem;background:${data.success?'#dcfce7':'#fef2f2'};color:${data.success?'#166534':'#991b1b'};border:1px solid ${data.success?'rgba(22,163,74,0.4)':'rgba(220,38,38,0.35)'};`;
            msgEl.textContent = data.message || (data.error || 'Error');
        }
        if (data.success || resp.ok) {
            if (silent) {
                showToast('Book files updated.', 'success');
                loadLibBooks();
            } else {
                showToast(editId ? 'Book updated!' : 'Book added!', 'success');
                setTimeout(() => { closeBookModal(); loadLibBooks(); }, 1200);
            }
        }
    } catch (e) { showToast('Save failed.', 'error'); }
}

// deleteBook: DELETE /api/books/:id
async function deleteBook(id, title) {
    if (!confirm(`Delete "${title}"? This cannot be undone.`)) return;
    const resp = await fetch(`/api/books/${id}`, { method: 'DELETE' });
    const data = await resp.json();
    showToast(data.message, data.success ? 'success' : 'error');
    if (data.success) loadLibBooks(); // Refresh list
}


async function loadAllBorrows() {
    const loading = document.getElementById('borrows-loading');
    const table   = document.getElementById('borrows-table');
    const body    = document.getElementById('borrows-body');
    try {
        const resp = await fetch('/api/librarian/borrows');
        const data = await resp.json();
        const loans = data.borrows || [];

        if (loading) loading.style.display = 'none';
        if (table)   table.style.display = 'table';
        if (body) {
            if (loans.length === 0) {
                body.innerHTML = '<tr><td colspan="8" class="text-center text-muted-custom py-4">No active borrows.</td></tr>';
            } else {
                body.innerHTML = loans.map(l => `
                    <tr>
                      <td style="font-weight:600;">${l.userName}</td>
                      <td style="font-size:0.78rem;color:#64748b;">${l.email}</td>
                      <td>${l.title}</td>
                      <td style="font-size:0.78rem;">${l.borrowDate?.split(' ')[0]}</td>
                      <td style="font-size:0.78rem;color:${l.status==='overdue'?'#dc2626':'#64748b'}">${l.dueDate?.split(' ')[0]}</td>
                      <td><span class="badge-${l.status}">${l.status.toUpperCase()}</span></td>
                      <td style="color:${l.fine>0?'#ea580c':'#64748b'}">${l.fine > 0 ? l.fine+' BDT' : '—'}</td>
                      <td><button onclick="processReturn(${l.id})" class="btn-sm-action btn-sm-green">Return</button></td>
                    </tr>`).join('');
            }
        }
    } catch (e) { showToast('Failed to load borrows.', 'error'); }
}

// processReturn: POST /api/librarian/return (uses ReturnCommand on server)
async function processReturn(txId) {
    if (!confirm('Process this return?')) return;
    const resp = await fetch('/api/librarian/return', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ transactionId: txId })
    });
    const data = await resp.json();
    showToast(data.message, data.success ? 'success' : 'error');
    if (data.success) loadAllBorrows(); // Refresh the table
}


let allStudents = []; // Cache students for client-side filtering
async function loadStudents() {
    const loading = document.getElementById('students-loading');
    const table   = document.getElementById('students-table');
    const body    = document.getElementById('students-body');
    try {
        const resp = await fetch('/api/librarian/students');
        const data = await resp.json();
        allStudents = data.students || [];
        if (loading) loading.style.display = 'none';
        if (table)   table.style.display = 'table';
        renderStudents(allStudents);
    } catch (e) { showToast('Failed to load students.', 'error'); }
}

function renderStudents(students) {
    const body = document.getElementById('students-body');
    if (!body) return;
    if (students.length === 0) {
        body.innerHTML = '<tr><td colspan="6" class="text-center text-muted-custom py-4">No students found.</td></tr>';
        return;
    }
    body.innerHTML = students.map(s => `
        <tr>
          <td style="font-weight:600;">${s.name}</td>
          <td style="font-size:0.78rem;color:#64748b;">${s.email}</td>
          <td>${s.membership.toUpperCase()}</td>
          <td style="color:${s.fineBalance>0?'#ea580c':'#64748b'}">${s.fineBalance > 0 ? s.fineBalance+' BDT' : '—'}</td>
          <td>${s.isSuspended ? '<span class="badge-overdue">SUSPENDED</span>' : '<span class="badge-active">ACTIVE</span>'}</td>
          <td style="font-size:0.78rem;color:#64748b;">${s.createdAt?.split(' ')[0]}</td>
        </tr>`).join('');
}

// filterStudents: Client-side search through cached student list
function filterStudents() {
    const q = document.getElementById('student-search')?.value.toLowerCase() || '';
    const filtered = allStudents.filter(s =>
        s.name.toLowerCase().includes(q) || s.email.toLowerCase().includes(q)
    );
    renderStudents(filtered);
}


async function loadStudentPicker() {
    try {
        const resp = await fetch('/api/librarian/students');
        const data = await resp.json();
        const students = data.students || [];
        const select = document.getElementById('notif-user');
        if (select) {
            select.innerHTML = students.map(s =>
                `<option value="${s.id}">${s.name} (${s.email})</option>`
            ).join('');
        }
    } catch (e) { showToast('Failed to load students.', 'error'); }
}

// useTemplate: Fill in a quick message template
function useTemplate(type) {
    const templates = {
        overdue:  '⚠️ Reminder: You have an overdue book. Please return it as soon as possible to avoid additional fines.',
        fine:     '💰 Your account has an outstanding fine. Please clear your balance to continue borrowing books.',
        welcome:  '👋 Welcome to SmartLibrary! Your account is now active. Explore our catalog to find your first book.'
    };
    const el = document.getElementById('notif-message');
    if (el) el.value = templates[type] || '';
}

// sendNotification: POST /api/librarian/notify (Observer Pattern)
async function sendNotification() {
    const userId  = document.getElementById('notif-user')?.value;
    const message = document.getElementById('notif-message')?.value;

    if (!userId || !message) {
        showToast('Select a student and write a message.', 'error');
        return;
    }

    const resp = await fetch('/api/librarian/notify', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ userId: parseInt(userId), message })
    });
    const data = await resp.json();
    const msgEl = document.getElementById('notif-msg');
    if (msgEl) {
        msgEl.style.cssText = `display:block;padding:0.75rem;border-radius:8px;background:${data.success?'#dcfce7':'#fef2f2'};color:${data.success?'#166534':'#991b1b'};border:1px solid ${data.success?'rgba(22,163,74,0.4)':'rgba(220,38,38,0.35)'};font-size:0.875rem;`;
        msgEl.textContent = data.message;
    }
    if (data.success) {
        showToast('Notification sent!', 'success');
        document.getElementById('notif-message').value = ''; // Clear the message
    }
}

// ============================================================
// Membership billing (librarian + admin)
// ============================================================
async function loadBillingInvoices() {
    const loading = document.getElementById('lib-billing-loading');
    const wrap = document.getElementById('lib-billing-table-wrap');
    const body = document.getElementById('lib-billing-body');
    const empty = document.getElementById('lib-billing-empty');
    const filter = document.getElementById('billing-filter')?.value || 'all';
    if (!body) return;
    if (loading) loading.style.display = 'block';
    if (wrap) wrap.style.display = 'none';
    if (empty) empty.style.display = 'none';
    try {
        const resp = await fetch('/api/librarian/billing?status=' + encodeURIComponent(filter));
        if (!resp.ok) throw new Error('load failed');
        const data = await resp.json();
        const bills = data.bills || [];
        if (loading) loading.style.display = 'none';
        if (bills.length === 0) {
            if (empty) empty.style.display = 'block';
            return;
        }
        if (wrap) wrap.style.display = 'block';
        body.innerHTML = bills.map(b => {
            const actions = b.status === 'pending'
                ? `<button type="button" onclick="markBillPaid(${b.id})" class="btn-sm-action btn-sm-green me-1">Mark paid</button>`
                  + `<button type="button" onclick="remindBill(${b.id})" class="btn-sm-action btn-sm-amber">Remind</button>`
                : '<span class="text-muted-custom" style="font-size:0.78rem;">—</span>';
            const st = b.status === 'pending'
                ? '<span class="badge-overdue">DUE</span>'
                : '<span class="badge-active">PAID</span>';
            return `<tr>
              <td>#${b.id}</td>
              <td style="font-weight:600;">${escapeHtml(b.userName)}</td>
              <td style="font-size:0.78rem;color:#64748b;">${escapeHtml(b.email)}</td>
              <td>${b.targetTier}</td>
              <td>${b.amountBdt} BDT</td>
              <td>${st}</td>
              <td style="font-size:0.78rem;color:#64748b;">${b.createdAt || '—'}</td>
              <td>${actions}</td>
            </tr>`;
        }).join('');
    } catch (e) {
        if (loading) loading.style.display = 'none';
        showToast('Failed to load billing.', 'error');
    }
}

function escapeHtml(s) {
    if (!s) return '';
    return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/"/g,'&quot;');
}

// ============================================================
// Seat management (librarian + admin) — /librarian/seats.html
// ============================================================
async function loadLibrarianSeatStudents() {
    const sel = document.getElementById('lib-seat-student');
    if (!sel) return;
    try {
        const resp = await fetch('/api/librarian/students');
        if (!resp.ok) throw new Error('denied');
        const data = await resp.json();
        const students = data.students || [];
        sel.innerHTML = students.length
            ? '<option value="">Choose a student…</option>' + students.map(s =>
                `<option value="${s.id}">${escapeHtml(s.name)} (${escapeHtml(s.email)})</option>`
              ).join('')
            : '<option value="">No students found</option>';
    } catch (e) {
        sel.innerHTML = '<option value="">Could not load students</option>';
        showToast('Failed to load students.', 'error');
    }
}

async function loadLibrarianSeatsPage() {
    const grid = document.getElementById('lib-seat-grid');
    const dateEl = document.getElementById('lib-seat-date');
    const loading = document.getElementById('lib-seat-table-loading');
    const wrap = document.getElementById('lib-seat-table-wrap');
    const tbody = document.getElementById('lib-seat-table-body');
    const empty = document.getElementById('lib-seat-table-empty');
    if (!grid || !dateEl) return;

    const date = dateEl.value || '';
    const slot = document.getElementById('lib-seat-timeslot')?.value || '';
    const room = document.getElementById('lib-seat-room')?.value || '';

    if (loading) loading.style.display = 'block';
    if (wrap) wrap.style.display = 'none';
    if (empty) empty.style.display = 'none';

    try {
        const resp = await fetch('/api/librarian/seats?date=' + encodeURIComponent(date));
        if (!resp.ok) throw new Error('load failed');
        const data = await resp.json();
        const bookings = data.bookings || [];

        const forCell = bookings.filter(b => b.room === room && b.timeSlot === slot);
        const bySeat = new Map(forCell.map(b => [b.seatNumber, b]));

        const seats = ['A1','A2','A3','A4','A5','B1','B2','B3','B4','B5'];
        grid.innerHTML = seats.map(s => {
            const b = bySeat.get(s);
            const taken = !!b;
            const title = taken
                ? `Booked: ${b.userName} (${b.email})`
                : 'Available';
            const label = taken ? `${s} · ${(b.userName || '').split(' ')[0] || '?'}` : s;
            return `<button type="button" onclick="libSelectSeat('${s}')" id="lib-seat-btn-${s}"
                title="${escapeHtml(title)}"
                style="width:auto;min-width:52px;height:48px;padding:0 8px;border-radius:8px;border:1px solid ${taken?'rgba(220,38,38,0.45)':'rgba(22,163,74,0.45)'};
                background:${taken?'#fef2f2':'#dcfce7'};
                color:${taken?'#dc2626':'#15803d'};font-size:0.72rem;font-weight:700;cursor:${taken?'not-allowed':'pointer'};"
                ${taken ? 'disabled' : ''}>${taken ? escapeHtml(label) : s}</button>`;
        }).join('');

        if (loading) loading.style.display = 'none';
        if (bookings.length === 0) {
            if (empty) empty.style.display = 'block';
            if (wrap) wrap.style.display = 'none';
        } else {
            if (empty) empty.style.display = 'none';
            if (wrap) wrap.style.display = 'block';
            if (tbody) {
                tbody.innerHTML = bookings.map(b => `
                  <tr>
                    <td style="font-size:0.82rem;">${escapeHtml(b.timeSlot)}</td>
                    <td>${escapeHtml(b.room)}</td>
                    <td style="font-weight:600;">${escapeHtml(b.seatNumber)}</td>
                    <td>${escapeHtml(b.userName)}</td>
                    <td style="font-size:0.78rem;color:#64748b;">${escapeHtml(b.email)}</td>
                  </tr>`).join('');
            }
        }
    } catch (e) {
        if (loading) loading.style.display = 'none';
        showToast('Failed to load seat bookings.', 'error');
    }
}

function libSelectSeat(seatNumber) {
    const inp = document.getElementById('lib-selected-seat');
    if (inp) inp.value = seatNumber;
    showToast(`Seat ${seatNumber} selected`, 'info');
    document.querySelectorAll('[id^="lib-seat-btn-"]').forEach(btn => { btn.style.boxShadow = ''; });
    const selectedBtn = document.getElementById(`lib-seat-btn-${seatNumber}`);
    if (selectedBtn && !selectedBtn.disabled) selectedBtn.style.boxShadow = '0 0 0 3px rgba(37,99,235,0.5)';
}

async function libBookSeatForStudent() {
    const seat = document.getElementById('lib-selected-seat')?.value;
    const date = document.getElementById('lib-seat-date')?.value;
    const timeSlot = document.getElementById('lib-seat-timeslot')?.value;
    const room = document.getElementById('lib-seat-room')?.value;
    const userId = document.getElementById('lib-seat-student')?.value;
    const msgEl = document.getElementById('lib-seat-msg');

    if (!userId) {
        showToast('Select a student.', 'error');
        return;
    }
    if (!seat) {
        showToast('Select an available seat.', 'error');
        return;
    }
    if (!date) {
        showToast('Select a date.', 'error');
        return;
    }

    try {
        const resp = await fetch('/api/librarian/seats/book', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                userId: parseInt(userId, 10),
                seatNumber: seat,
                room,
                date,
                timeSlot
            })
        });
        let data = {};
        try { data = await resp.json(); } catch (_) {}
        if (msgEl) {
            msgEl.style.display = 'block';
            const ok = resp.ok && data.success;
            msgEl.style.cssText = `display:block;padding:0.75rem;border-radius:8px;font-size:0.875rem;background:${ok?'#dcfce7':'#fef2f2'};color:${ok?'#166534':'#991b1b'};border:1px solid ${ok?'rgba(22,163,74,0.4)':'rgba(220,38,38,0.35)'};`;
            msgEl.textContent = data.message || data.error || (ok ? 'Booked.' : 'Booking failed.');
        }
        if (resp.ok && data.success) {
            showToast('Seat booked for student.', 'success');
            document.getElementById('lib-selected-seat').value = '';
            loadLibrarianSeatsPage();
        }
    } catch (e) {
        showToast('Booking failed.', 'error');
    }
}

async function markBillPaid(billId) {
    if (!confirm('Record payment for invoice #' + billId + '? Student membership will update.')) return;
    try {
        const resp = await fetch('/api/librarian/billing/mark-paid', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ billId })
        });
        const data = await resp.json();
        showToast(data.message || (data.success ? 'Saved.' : 'Failed.'), data.success ? 'success' : 'error');
        if (data.success) loadBillingInvoices();
    } catch (e) {
        showToast('Request failed.', 'error');
    }
}

async function remindBill(billId) {
    try {
        const resp = await fetch('/api/librarian/billing/remind', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ billId })
        });
        const data = await resp.json();
        showToast(data.message || (data.success ? 'Reminder sent.' : 'Failed.'), data.success ? 'success' : 'error');
    } catch (e) {
        showToast('Request failed.', 'error');
    }
}
