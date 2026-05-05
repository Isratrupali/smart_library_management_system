
function switchTab(tab) {
    // Get references to both form containers
    const loginForm    = document.getElementById('form-login');    // The login form div
    const registerForm = document.getElementById('form-register'); // The register form div
    const tabLogin     = document.getElementById('tab-login');     // Login tab button
    const tabRegister  = document.getElementById('tab-register');  // Register tab button

    // Active tab style: gradient background (visually selected)
    const activeStyle   = 'background:linear-gradient(135deg,#2563eb,#16a34a);color:white;';
    // Inactive tab style: transparent background (not selected)
    const inactiveStyle = 'background:transparent;color:#64748b;';

    if (tab === 'login') {
        loginForm.style.display    = 'block';  // Show the login form
        registerForm.style.display = 'none';   // Hide the register form
        tabLogin.style.cssText     = activeStyle + 'flex:1;padding:0.6rem;border:none;border-radius:8px;font-weight:600;font-size:0.875rem;cursor:pointer;transition:all 0.2s;';
        tabRegister.style.cssText  = inactiveStyle + 'flex:1;padding:0.6rem;border:none;border-radius:8px;font-weight:600;font-size:0.875rem;cursor:pointer;transition:all 0.2s;';
    } else {
        loginForm.style.display    = 'none';   // Hide the login form
        registerForm.style.display = 'block';  // Show the register form
        tabRegister.style.cssText  = activeStyle + 'flex:1;padding:0.6rem;border:none;border-radius:8px;font-weight:600;font-size:0.875rem;cursor:pointer;transition:all 0.2s;';
        tabLogin.style.cssText     = inactiveStyle + 'flex:1;padding:0.6rem;border:none;border-radius:8px;font-weight:600;font-size:0.875rem;cursor:pointer;transition:all 0.2s;';
    }
}


function fillDemo(email) {
    document.getElementById('login-email').value    = email;        // Set the email field
    document.getElementById('login-password').value = 'password123'; // All demos use this password
    loginUser(); // Auto-submit the login form immediately after filling
}


function showMsg(msgId, text, type) {
    const el = document.getElementById(msgId); // Find the message container element
    el.style.display = 'block';                // Make it visible (was display:none)

    // Set background and text color based on message type
    const isSuccess = type === 'success';
    el.style.cssText = `
        display:block;
        padding:0.75rem 1rem;
        border-radius:8px;
        font-size:0.875rem;
        background:${isSuccess ? '#dcfce7' : '#fef2f2'};
        border:1px solid ${isSuccess ? 'rgba(22,163,74,0.4)' : 'rgba(220,38,38,0.35)'};
        color:${isSuccess ? '#166534' : '#991b1b'};
    `;
    el.textContent = text; // Set the message text content
}


async function loginUser() {
    // Read values from the form input fields
    const email    = document.getElementById('login-email').value.trim();    // Trim whitespace
    const password = document.getElementById('login-password').value;

    // Basic client-side validation before hitting the server
    if (!email || !password) {
        showMsg('login-msg', 'Please enter your email and password.', 'error');
        return; // Stop here — don't make the API call
    }

    try {
        // fetch: make an HTTP POST request to the login API endpoint
        const response = await fetch('/api/login', {
            method: 'POST',                           // HTTP method
            headers: { 'Content-Type': 'application/json' }, // Tell server we're sending JSON
            body: JSON.stringify({ email, password }) // Serialize the data to JSON string
        });

        // Parse the JSON response body
        const data = await response.json();

        if (response.ok && data.success) {
            // Login succeeded! Save basic info to localStorage for UI display
            localStorage.setItem('slms_user', JSON.stringify({
                name:   data.name,   // Store name for displaying in navbar/dashboard
                role:   data.role,   // Store role for conditional UI elements
                userId: data.userId  // Store user ID for API calls
            }));

            showMsg('login-msg', 'Login successful! Redirecting...', 'success');

            // Redirect to the correct dashboard based on the user's role
            // data.dashboard contains the path returned by the Factory Pattern
            setTimeout(() => {
                window.location.href = data.dashboard; // Go to role-specific dashboard
            }, 600); // Short delay so user sees the success message
        } else {
            // Login failed — show the error message from the server
            showMsg('login-msg', data.error || 'Login failed. Check your credentials.', 'error');
        }
    } catch (err) {
        // Network error or server not running
        showMsg('login-msg', 'Cannot connect to server. Is slms.exe running?', 'error');
    }
}


async function registerUser() {
    // Read all registration form field values
    const name     = document.getElementById('reg-name').value.trim();
    const email    = document.getElementById('reg-email').value.trim();
    const password = document.getElementById('reg-password').value;
    const confirm  = document.getElementById('reg-confirm').value;

    // Client-side validation: all fields are required
    if (!name || !email || !password || !confirm) {
        showMsg('register-msg', 'All fields are required.', 'error');
        return;
    }

    // Check password minimum length
    if (password.length < 6) {
        showMsg('register-msg', 'Password must be at least 6 characters.', 'error');
        return;
    }

    // Check passwords match before sending to server
    if (password !== confirm) {
        showMsg('register-msg', 'Passwords do not match.', 'error');
        return;
    }

    try {
        // POST the registration data to the server
        const response = await fetch('/api/register', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name, email, password }) // Send name, email, password
        });

        const data = await response.json();

        if (response.ok && data.success) {
            // Registration succeeded — show success and switch to login tab
            showMsg('register-msg', '✅ Account created! Please login.', 'success');
            // Pre-fill the login form with the registered email
            document.getElementById('login-email').value = email;
            // Switch to login tab after a short delay
            setTimeout(() => switchTab('login'), 1500);
        } else {
            // Registration failed — show server error message
            showMsg('register-msg', data.error || 'Registration failed.', 'error');
        }
    } catch (err) {
        showMsg('register-msg', 'Cannot connect to server. Is slms.exe running?', 'error');
    }
}


(async function checkAlreadyLoggedIn() {
    try {
        const resp = await fetch('/api/me'); // Check current session
        if (resp.ok) {
            const data = await resp.json();
            if (data.loggedIn) {
                // Already logged in — redirect to their dashboard
                const paths = {
                    student:   '/student/dashboard.html',
                    librarian: '/librarian/dashboard.html',
                    admin:     '/admin/dashboard.html'
                };
                window.location.href = paths[data.role] || '/'; // Redirect
            }
        }
    } catch (e) {
        // Ignore errors (server might not be ready yet)
    }
})(); // Immediately-invoked: runs as soon as this script loads
