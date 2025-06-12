function checkModel() {
    const model = document.getElementById("modelInput").value.toUpperCase();
    const result = document.getElementById("result");

    if (model.length < 9) {
        result.textContent = "Please enter a valid model number.";
        return;
    }

    const classification = model.substring(0, 2);
    const capacity = model.substring(2, 5);
    const productType = model.charAt(6);
    const productNotation = model.charAt(7);
    const feature = model.charAt(8);
    const ratingVoltage = model.charAt(9);
    const mode = model.charAt(10);

    let type = "Other";
    if (productType === "S" || productType === "N" || productType === "X") {
        type = "NASA";
    } else if (productType === "A" || productType === "B" || productType === "C") {
        type = "Non-NASA";
    }

    result.innerHTML = `
        <div class="result-container">
          <p><strong>Model:</strong> ${model}</p>
          <p><strong>Classification:</strong> ${classification}</p>
          <p><strong>Capacity:</strong> ${capacity} kW</p>
          <p><strong>Product Type:</strong> <span class="highlight">${productType} (${type})</span></p>
          <p><strong>Product Notation:</strong> ${productNotation}</p>
          <p><strong>Feature:</strong> ${feature}</p>
          <p><strong>Rating Voltage:</strong> ${ratingVoltage}</p>
          <p><strong>Mode:</strong> ${mode}</p>
        </div>
    `;
}
