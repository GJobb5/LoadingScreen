/* ==========================================================================
   Spark Global Loading Screen - Core JavaScript Logic & Animation Engine
   ========================================================================== */

(function () {
  'use strict';

  // State
  let currentProgress = 0;
  let targetProgress = 0;
  let isCompleted = false;

  // DOM Elements
  const progressBar = document.getElementById('progressBar');
  const progressPercent = document.getElementById('progressPercent');
  const statusMessage = document.getElementById('statusMessage');
  const tipText = document.getElementById('tipText');
  const loadingWrapper = document.getElementById('loadingWrapper');
  const canvas = document.getElementById('bgCanvas');
  const ctx = canvas.getContext('2d');

  // Rotating Tips List
  const tips = [
    'ยินดีต้อนรับสู่ Spark Global เตรียมพร้อมสำหรับความมันส์ระดับพรีเมียม!',
    'กดปุ่ม F1 เพื่อดูคู่มือช่วยเหลือและคำสั่งเบื้องต้นภายในเซิร์ฟเวอร์',
    'ร่วมพูดคุยและอัปเดตกิจกรรมล่าสุดได้ที่ Discord: discord.gg/spark',
    'อย่าลืมตรวจสอบกฎของเซิร์ฟเวอร์ เพื่อประสบการณ์ Roleplay ที่ดีร่วมกัน',
    'ระบบเสียง 3D Voice จะทำงานอัตโนมัติตามระยะห่างของตัวละคร',
    'พบเจอปัญหาหรือบั๊ก แจ้งแอดมินผ่านระบบตั๋วใน Discord ได้ตลอด 24 ชม.'
  ];

  let currentTipIndex = 0;

  function rotateTips() {
    if (!tipText) return;
    tipText.style.opacity = '0';
    tipText.style.transform = 'translateY(5px)';

    setTimeout(() => {
      currentTipIndex = (currentTipIndex + 1) % tips.length;
      tipText.textContent = tips[currentTipIndex];
      tipText.style.opacity = '1';
      tipText.style.transform = 'translateY(0)';
    }, 400);
  }

  setInterval(rotateTips, 5000);

  // ==========================================
  // Interactive Particle System
  // ==========================================
  let width, height;
  let particles = [];
  const PARTICLE_COUNT = 65;

  function resizeCanvas() {
    width = canvas.width = window.innerWidth;
    height = canvas.height = window.innerHeight;
  }

  window.addEventListener('resize', resizeCanvas);
  resizeCanvas();

  class Particle {
    constructor() {
      this.reset(true);
    }

    reset(initial = false) {
      this.x = Math.random() * width;
      this.y = initial ? Math.random() * height : height + 10;
      this.radius = Math.random() * 2.2 + 0.8;
      this.speedY = Math.random() * 0.8 + 0.3;
      this.speedX = (Math.random() - 0.5) * 0.4;
      this.opacity = Math.random() * 0.7 + 0.2;
      this.color = Math.random() > 0.4 ? '#ff4081' : (Math.random() > 0.5 ? '#00e5ff' : '#ffffff');
    }

    update() {
      this.y -= this.speedY;
      this.x += this.speedX;

      if (this.y < -10 || this.x < -10 || this.x > width + 10) {
        this.reset();
      }
    }

    draw() {
      ctx.save();
      ctx.beginPath();
      ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2);
      ctx.fillStyle = this.color;
      ctx.globalAlpha = this.opacity;
      ctx.shadowBlur = 10;
      ctx.shadowColor = this.color;
      ctx.fill();
      ctx.restore();
    }
  }

  for (let i = 0; i < PARTICLE_COUNT; i++) {
    particles.push(new Particle());
  }

  function drawConnections() {
    const maxDist = 110;
    for (let i = 0; i < particles.length; i++) {
      for (let j = i + 1; j < particles.length; j++) {
        const dx = particles[i].x - particles[j].x;
        const dy = particles[i].y - particles[j].y;
        const dist = Math.sqrt(dx * dx + dy * dy);

        if (dist < maxDist) {
          ctx.beginPath();
          ctx.moveTo(particles[i].x, particles[i].y);
          ctx.lineTo(particles[j].x, particles[j].y);
          ctx.strokeStyle = '#ff4081';
          ctx.globalAlpha = (1 - dist / maxDist) * 0.15;
          ctx.stroke();
        }
      }
    }
  }

  function animate() {
    ctx.clearRect(0, 0, width, height);

    drawConnections();

    for (let i = 0; i < particles.length; i++) {
      particles[i].update();
      particles[i].draw();
    }

    // Smooth Progress Interpolation
    if (currentProgress < targetProgress) {
      currentProgress += (targetProgress - currentProgress) * 0.1;
      if (Math.abs(targetProgress - currentProgress) < 0.1) {
        currentProgress = targetProgress;
      }
      updateUI();
    }

    requestAnimationFrame(animate);
  }

  function updateUI() {
    const pct = Math.min(100, Math.max(0, Math.round(currentProgress)));
    if (progressBar) progressBar.style.width = `${pct}%`;
    if (progressPercent) progressPercent.textContent = pct;
  }

  // ==========================================
  // Public C++ Bridge API
  // ==========================================
  window.setLoadingProgress = function (progress, message) {
    targetProgress = Math.max(0, Math.min(100, progress));
    if (message && statusMessage) {
      statusMessage.textContent = message;
    }
  };

  window.setGameLoaded = function () {
    targetProgress = 100;
    currentProgress = 100;
    updateUI();
    if (statusMessage) statusMessage.textContent = 'เข้าสู่เกมเรียบร้อยแล้ว!';
    if (loadingWrapper) {
      setTimeout(() => {
        loadingWrapper.classList.add('fade-out');
      }, 500);
    }
  };

  // Start Animation
  animate();

  // Initial simulated progress for demo/fallback when standalone
  window.setLoadingProgress(15, 'กำลังโหลดโมเดลและพื้นผิวเกม...');
})();
