
async function checkAdmin() {
    try {
        const resp = await fetch('/api/me');  // Ask server for current session
        if (!resp.ok) { window.location.href = '/auth.html'; return; }
        const data = await resp.json();
        // Admins can also access librarian functions, but not vice versa
        if (!data.loggedIn || data.role !== 'admin') {
            window.location.href = '/auth.html'; // Redirect if not admin
        }
    } catch (e) { window.location.href = '/auth.html'; }
}

// logout: Clear session and redirect to login page
async function logout() {
    await fetch('/api/logout', { method: 'POST' }); // Invalidate server session
    window.location.href = '/auth.html';
}

// showToast: Brief popup notification message (top-right corner)
function showToast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    if (!container) return;
    const toast = document.createElement('div');
    toast.className = `toast-msg toast-${type}`; // CSS handles color per type
    toast.textContent = message;
    container.appendChild(toast);
    setTimeout(() => toast.remove(), 3500); // Auto-remove after 3.5s
}


async function loadAdminStats() {
    try {
        const resp = await fetch('/api/admin/stats');
        const s = await resp.json();

        // Helper to safely set an element's text content
        const set = (id, val) => {
            const el = document.getElementById(id);
            if (el) el.textContent = val;
        };

        set('stat-books',     s.totalBooks     || 0);
        set('stat-users',     s.totalUsers     || 0);
        set('stat-students',  s.totalStudents  || 0);
        set('stat-librarians',s.totalLibrarians|| 0);
        set('stat-active',    s.activeLoans    || 0);
        set('stat-overdue',   s.overdueLoans   || 0);
        set('stat-fines',     (s.totalFines || 0) + ' BDT');
        set('stat-seats',     s.activeSeats    || 0);
        set('stat-pending-bills', s.pendingBills ?? 0);
    } catch (e) { showToast('Failed to load stats.', 'error'); }
}


async function loadStats() {
    try {
        const resp = await fetch('/api/admin/stats');
        const s = await resp.json();

        // Fill the summary stat cards at the bottom of the stats page
        const set = (id, val) => { const el = document.getElementById(id); if (el) el.textContent = val; };
        set('s-books',   s.totalBooks  || 0);
        set('s-users',   s.totalUsers  || 0);
        set('s-fines',   (s.totalFines || 0) + ' BDT');
        set('s-overdue', s.overdueLoans || 0);

        // ---- CHART 1: Loan Status Doughnut Chart ----
        // Shows the proportion of active vs overdue loans
        const loanCtx = document.getElementById('loanChart');
        if (loanCtx) {
            new Chart(loanCtx, {
                type: 'doughnut', // Doughnut chart type
                data: {
                    labels: ['Active Loans', 'Overdue Loans'],
                    datasets: [{
                        data: [s.activeLoans || 0, s.overdueLoans || 0],
                        // Match colors to the app's design tokens
                        backgroundColor: ['rgba(22,163,74,0.75)', 'rgba(220,38,38,0.7)'],
                        borderColor:     ['rgba(22,163,74,1)',   'rgba(220,38,38,1)'],
                        borderWidth: 2
                    }]
                },
                options: {
                    responsive: true,
                    plugins: {
                        legend: {
                            // Style the legend to match the dark theme
                            labels: { color: '#475569', font: { size: 12 } }
                        }
                    }
                }
            });
        }

        // ---- CHART 2: User Role Bar Chart ----
        // Shows how many students, librarians, and admins there are
        const userCtx = document.getElementById('userChart');
        if (userCtx) {
            new Chart(userCtx, {
                type: 'bar', // Vertical bar chart
                data: {
                    labels: ['Students', 'Librarians', 'Admins'],
                    datasets: [{
                        label: 'Number of Users',
                        data: [s.totalStudents || 0, s.totalLibrarians || 0, 1],
                        backgroundColor: [
                            'rgba(37, 99, 235, 0.65)',
                            'rgba(249, 115, 22, 0.65)',
                            'rgba(22, 163, 74, 0.65)'
                        ],
                        borderColor: [
                            'rgba(37, 99, 235, 1)',
                            'rgba(234, 88, 12, 1)',
                            'rgba(22, 163, 74, 1)'
                        ],
                        borderWidth: 2,
                        borderRadius: 6 // Rounded bar tops
                    }]
                },
                options: {
                    responsive: true,
                    plugins: {
                        legend: { display: false } // Hide legend (labels are on x-axis)
                    },
                    scales: {
                        y: {
                            beginAtZero: true, // Start y-axis at 0
                            ticks: { color: '#64748b' },
                            grid:  { color: 'rgba(15,23,42,0.08)' }
                        },
                        x: {
                            ticks: { color: '#64748b' },
                            grid:  { color: 'rgba(15,23,42,0.06)' }
                        }
                    }
                }
            });
        }
    } catch (e) { showToast('Failed to load statistics.', 'error'); }
}


let allUsers = []; // Cache all users for client-side search filtering

async function loadAdminUsers() {
    const roleFilter  = document.getElementById('user-role-filter')?.value || '';
    const loadingEl   = document.getElementById('users-loading');
    const tableEl     = document.getElementById('users-table');
    const bodyEl      = document.getElementById('users-body');

    if (loadingEl) loadingEl.style.display = 'block';
    if (tableEl)   tableEl.style.display   = 'none';

    try {
        const url  = roleFilter ? `/api/admin/users?role=${roleFilter}` : '/api/admin/users';
        const resp = await fetch(url);
        const data = await resp.json();
        allUsers = data.users || [];

        if (loadingEl) loadingEl.style.display = 'none';
        if (tableEl)   tableEl.style.display   = 'table';
        renderUsersTable(allUsers); // Render the fetched users
    } catch (e) {
        if (loadingEl) loadingEl.style.display = 'none';
        showToast('Failed to load users.', 'error');
    }
}

// renderUsersTable: Builds the HTML rows for the users table
function renderUsersTable(users) {
    const body = document.getElementById('users-body');
    if (!body) return;

    if (users.length === 0) {
        body.innerHTML = '<tr><td colspan="7" class="text-center text-muted-custom py-4">No users found.</td></tr>';
        return;
    }

    body.innerHTML = users.map(u => {
        // Role badge: color-coded by role
        const roleBadge = u.role === 'admin'
            ? `<span class="role-badge role-admin">Admin</span>`
            : u.role === 'librarian'
                ? `<span class="role-badge role-librarian">Librarian</span>`
                : `<span class="role-badge role-student">Student</span>`;

        return `<tr>
          <td style="font-weight:600;">${u.name}</td>
          <td style="font-size:0.78rem;color:#64748b;">${u.email}</td>
          <td>${roleBadge}</td>
          <td>${u.membership ? u.membership.toUpperCase() : '—'}</td>
          <td style="color:${u.fineBalance > 0 ? '#ea580c' : '#64748b'}">${u.fineBalance > 0 ? u.fineBalance + ' BDT' : '—'}</td>
          <td>${u.isSuspended
              ? '<span class="badge-overdue">SUSPENDED</span>'
              : '<span class="badge-active">ACTIVE</span>'}</td>
          <td>
            <button onclick='openEditUserModal(${JSON.stringify(u).replace(/'/g,"\\'")})'
              class="btn-sm-action btn-sm-cyan me-1">Edit</button>
            <button onclick="deleteAdminUser(${u.id},'${u.name.replace(/'/g, "\\'")}')"
              class="btn-sm-action btn-sm-red">Delete</button>
          </td>
        </tr>`;
    }).join('');
}

// filterUsers: Client-side search through cached user list
function filterUsers() {
    const q = document.getElementById('user-search')?.value.toLowerCase() || '';
    const filtered = allUsers.filter(u =>
        u.name.toLowerCase().includes(q) || u.email.toLowerCase().includes(q)
    );
    renderUsersTable(filtered); // Re-render with filtered results only
}


function openAddUserModal() {
    // Clear all form fields before showing the modal
    ['au-name','au-email','au-password'].forEach(id => {
        const el = document.getElementById(id); if (el) el.value = '';
    });
    const msg = document.getElementById('au-msg');
    if (msg) msg.style.display = 'none';
    document.getElementById('add-user-modal').classList.add('active'); // Show modal
}

function closeAddUserModal() {
    document.getElementById('add-user-modal').classList.remove('active'); // Hide modal
}

// createUser: POST /api/admin/users — uses Factory Pattern on backend for role validation
async function createUser() {
    const name     = document.getElementById('au-name').value.trim();
    const email    = document.getElementById('au-email').value.trim();
    const password = document.getElementById('au-password').value;
    const role     = document.getElementById('au-role').value;  // "student" | "librarian" | "admin"

    if (!name || !email || !password) {
        showToast('Name, email, and password are required.', 'error'); return;
    }

    try {
        const resp = await fetch('/api/admin/users', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name, email, password, role }) // Factory validates role on server
        });
        const data = await resp.json();
        const msgEl = document.getElementById('au-msg');
        if (msgEl) {
            msgEl.style.cssText = `display:block;padding:0.75rem;border-radius:8px;font-size:0.875rem;background:${data.success?'#dcfce7':'#fef2f2'};color:${data.success?'#166534':'#991b1b'};border:1px solid ${data.success?'rgba(22,163,74,0.4)':'rgba(220,38,38,0.35)'};`;
            msgEl.textContent = data.message || data.error;
        }
        if (data.success) {
            showToast('User created!', 'success');
            setTimeout(() => { closeAddUserModal(); loadAdminUsers(); }, 1200);
        }
    } catch (e) { showToast('Create failed.', 'error'); }
}


function openEditUserModal(user) {
    if (typeof user === 'string') user = JSON.parse(user);
    document.getElementById('eu-id').value            = user.id;
    document.getElementById('eu-role').value          = user.role;
    document.getElementById('eu-membership').value    = user.membership || 'basic';
    document.getElementById('eu-fine').value          = user.fineBalance || 0;
    document.getElementById('eu-suspended').checked   = user.isSuspended;
    const msg = document.getElementById('eu-msg');
    if (msg) msg.style.display = 'none';
    document.getElementById('edit-user-modal').classList.add('active');
}

function closeEditUserModal() {
    document.getElementById('edit-user-modal').classList.remove('active');
}

// saveUserEdit: PUT /api/admin/users/:id — updates role, membership, suspension, fine
async function saveUserEdit() {
    const id         = document.getElementById('eu-id').value;
    const role       = document.getElementById('eu-role').value;
    const membership = document.getElementById('eu-membership').value;
    const fine       = parseFloat(document.getElementById('eu-fine').value) || 0;
    const suspended  = document.getElementById('eu-suspended').checked;

    try {
        const resp = await fetch(`/api/admin/users/${id}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ role, membership, fineBalance: fine, isSuspended: suspended })
        });
        const data = await resp.json();
        const msgEl = document.getElementById('eu-msg');
        if (msgEl) {
            msgEl.style.cssText = `display:block;padding:0.75rem;border-radius:8px;font-size:0.875rem;background:${data.success?'#dcfce7':'#fef2f2'};color:${data.success?'#166534':'#991b1b'};border:1px solid ${data.success?'rgba(22,163,74,0.4)':'rgba(220,38,38,0.35)'};`;
            msgEl.textContent = data.message;
        }
        if (data.success) {
            showToast('User updated!', 'success');
            setTimeout(() => { closeEditUserModal(); loadAdminUsers(); }, 1000);
        }
    } catch (e) { showToast('Update failed.', 'error'); }
}

// deleteAdminUser: DELETE /api/admin/users/:id
async function deleteAdminUser(id, name) {
    if (!confirm(`Permanently delete "${name}"? This cannot be undone.`)) return;
    try {
        const resp = await fetch(`/api/admin/users/${id}`, { method: 'DELETE' });
        const data = await resp.json();
        showToast(data.message, data.success ? 'success' : 'error');
        if (data.success) loadAdminUsers(); // Refresh table
    } catch (e) { showToast('Delete failed.', 'error'); }
}


async function loadMembershipAdmin() {
    try {
        const resp = await fetch('/api/admin/users?role=student');
        const data = await resp.json();
        const students = data.users || [];

        // Count students in each tier
        const counts = { basic: 0, premium: 0, gold: 0 };
        students.forEach(s => {
            if (counts[s.membership] !== undefined) counts[s.membership]++;
        });

        // Update the count display in each plan card
        ['basic','premium','gold'].forEach(tier => {
            const el = document.getElementById(`count-${tier}`);
            if (el) el.textContent = counts[tier];
        });

        // Populate the student membership table
        const loading = document.getElementById('membership-loading');
        const table   = document.getElementById('membership-table');
        const body    = document.getElementById('membership-body');

        if (loading) loading.style.display = 'none';
        if (table)   table.style.display   = 'table';
        if (body) {
            body.innerHTML = students.map(s => `
                <tr>
                  <td style="font-weight:600;">${s.name}</td>
                  <td style="font-size:0.78rem;color:#64748b;">${s.email}</td>
                  <td>${s.membership.toUpperCase()}</td>
                  <td style="color:${s.fineBalance > 0 ? '#ea580c' : '#64748b'}">${s.fineBalance > 0 ? s.fineBalance + ' BDT' : '—'}</td>
                  <td>
                    <!-- Upgrade/downgrade membership directly from this table -->
                    <select onchange="upgradeMembership(${s.id}, this.value)" style="background:#ffffff;border:1px solid rgba(37,99,235,0.2);color:#0f172a;border-radius:6px;padding:4px 8px;font-size:0.78rem;">
                      <option value="basic"   ${s.membership==='basic'   ? 'selected':''}>Basic</option>
                      <option value="premium" ${s.membership==='premium' ? 'selected':''}>Premium</option>
                      <option value="gold"    ${s.membership==='gold'    ? 'selected':''}>Gold</option>
                    </select>
                  </td>
                </tr>`).join('');
        }
    } catch (e) { showToast('Failed to load membership data.', 'error'); }
}

// upgradeMembership: Update a student's membership tier via admin PUT
async function upgradeMembership(userId, newTier) {
    try {
        const resp = await fetch(`/api/admin/users/${userId}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ membership: newTier }) // Only update membership field
        });
        const data = await resp.json();
        showToast(data.success ? `Membership updated to ${newTier}!` : 'Update failed.', data.success ? 'success' : 'error');
    } catch (e) { showToast('Update failed.', 'error'); }
}
