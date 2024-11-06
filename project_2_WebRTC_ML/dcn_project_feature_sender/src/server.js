const express = require('express');
const path = require('path');
const app = express();

// TO DO: you need to change the port 80 to allowed ports
const port = process.env.PORT || 62123;

app.use(express.static(path.join(__dirname, 'public')));

app.listen(port, () => {
  console.log(`Server running at http://localhost:${port}`);
});
