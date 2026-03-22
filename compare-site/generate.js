const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const dayFolders = fs.readdirSync('.')
  .filter(f => f.match(/^day\d+$/))
  .sort((a, b) => parseInt(a.replace('day', '')) - parseInt(b.replace('day', '')));

console.log('Found days:', dayFolders.join(', '));

fs.mkdirSync('compare-site/diffs', { recursive: true });

const template = fs.readFileSync('compare-site/template.html', 'utf8');

const excludeFlags = [
  '--exclude=Makefile',
  '--exclude=README.md',
  '--exclude=*.o',
  '--exclude=*.d',
  '--exclude=build',
  '--exclude=server',
  '--exclude=*.log',
  '--exclude=.DS_Store'
].join(' ');

for (let i = 1; i < dayFolders.length; i++) {
  const prevDay = dayFolders[i - 1];
  const currDay = dayFolders[i];
  const diffFileName = `${prevDay}-to-${currDay}.html`;
  const outputPath = path.join('compare-site/diffs', diffFileName);
  
  console.log(`Generating diff: ${prevDay} -> ${currDay}`);
  
  let diffOutput = '';
  try {
    diffOutput = execSync(`diff -ru ${excludeFlags} "${prevDay}" "${currDay}" 2>/dev/null`, {
      encoding: 'utf8',
      maxBuffer: 50 * 1024 * 1024,
      cwd: process.cwd()
    });
  } catch (e) {
    diffOutput = e.stdout || '';
  }
  
  const summary = generateSummary(diffOutput);
  
  const page = template
    .replaceAll('{{PREV_DAY}}', prevDay)
    .replaceAll('{{CURR_DAY}}', currDay)
    .replace('{{PREV_LINK}}', i > 1 ? `${dayFolders[i-2]}-to-${prevDay}.html` : '#')
    .replace('{{NEXT_LINK}}', i < dayFolders.length - 1 ? `${currDay}-to-${dayFolders[i+1]}.html` : '#')
    .replace('{{PREV_DISABLED}}', i > 1 ? '' : 'disabled')
    .replace('{{NEXT_DISABLED}}', i < dayFolders.length - 1 ? '' : 'disabled')
    .replace('{{NAV_INFO}}', `Day ${i} of ${dayFolders.length - 1}`)
    .replace('{{SUMMARY}}', summary)
    .replace('{{DIFF_JSON}}', JSON.stringify(diffOutput));
  
  fs.writeFileSync(outputPath, page);
  console.log(`  -> Created ${diffFileName} (${diffOutput.split('\n').length} lines)`);
}

console.log('\nGenerating index page...');

const indexHtml = generateIndexPage(dayFolders);
fs.writeFileSync('compare-site/index.html', indexHtml);

console.log('Done! Files generated in compare-site/');

function generateSummary(diffOutput) {
  if (!diffOutput.trim()) return 'No changes';
  
  const lines = diffOutput.split('\n');
  let filesChanged = 0;
  let insertions = 0;
  let deletions = 0;
  
  for (const line of lines) {
    if (line.startsWith('diff ')) filesChanged++;
    if (line.startsWith('+') && !line.startsWith('+++')) insertions++;
    if (line.startsWith('-') && !line.startsWith('---')) deletions++;
  }
  
  return `${filesChanged} files changed, ${insertions} insertions(+), ${deletions} deletions(-)`;
}

function generateIndexPage(days) {
  return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Day-by-Day Diff Viewer</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: #0d1117;
      color: #c9d1d9;
      min-height: 100vh;
      padding: 2rem;
    }
    .container { max-width: 1000px; margin: 0 auto; }
    h1 {
      font-size: 2rem;
      margin-bottom: 0.5rem;
      color: #58a6ff;
    }
    .subtitle {
      color: #8b949e;
      margin-bottom: 2rem;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
      gap: 1rem;
    }
    .card {
      background: #161b22;
      border: 1px solid #30363d;
      border-radius: 6px;
      padding: 1.5rem;
      text-decoration: none;
      transition: all 0.2s;
    }
    .card:hover {
      border-color: #58a6ff;
      transform: translateY(-2px);
    }
    .card-title {
      font-size: 1.25rem;
      font-weight: 600;
      color: #58a6ff;
      margin-bottom: 0.5rem;
    }
    .card-arrow {
      color: #8b949e;
      font-size: 0.875rem;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Day-by-Day Diff Viewer</h1>
    <p class="subtitle">Compare consecutive days in the 30-day server project</p>
    <div class="grid">
${days.slice(1).map((day, i) => `
      <a class="card" href="diffs/${days[i]}-to-${day}.html">
        <div class="card-title">${days[i]} → ${day}</div>
        <div class="card-arrow">Day ${i + 1} → Day ${i + 2}</div>
      </a>
`).join('\n')}
    </div>
  </div>
</body>
</html>`;
}
