const UIctrl = (function () {
  const UISelectors = {
    suggestFlag: ".search-ctrl .input-field .suggestions",
    resultFlag: ".search-result",
    copyBtn: ".copy-btn",
    searchInput: "#flag",
  };

  return {
    getUISelectors: function () {
      return UISelectors;
    },
  };
})();

const APPctrl = (function (UIctrl) {
  // * fetching data from restcountries.com
  const loadJSONData = async function () {
    const response = await fetch("https://restcountries.com/v3.1/all");
    const data = await response.json();
    const names = [];
    data.forEach((element) => {
      names.push(element["name"]["common"]);
    });

    return {
      data: data,
      names: names,
    };
  };

  //* loading event listeners
  const loadEventListeners = function () {
    // * search input key down event
    document
      .querySelector(UIctrl.getUISelectors().searchInput)
      .addEventListener("keyup", searchCountryKeyDown);

    // * list selection event
    document
      .querySelector(UIctrl.getUISelectors().suggestFlag)
      .addEventListener("click", listCountryClick);

    //* copy button click event
    document
      .querySelector(UIctrl.getUISelectors().copyBtn)
      .addEventListener("click", textCopyClick);
  };

  const searchCountryKeyDown = function (e) {
    flagName = e.target.value.toLowerCase();
    if (flagName === "") {
      document.querySelector(
        UIctrl.getUISelectors().suggestFlag
      ).style.display = "none";
    } else {
      loadJSONData().then((data) => {
        const suggestions = [];
        data.names.forEach((item) => {
          if (item.toLowerCase().indexOf(flagName) != -1) {
            suggestions.push(item);
          }
        });
        const suggest = document.querySelector(
          UIctrl.getUISelectors().suggestFlag
        );
        suggest.style.display = "block";
        suggest.innerHTML = "";
        suggestions.forEach((item) => {
          suggest.innerHTML += `
                    <li>${item}</li>
                `;
        });
      });
    }
  };

  const listCountryClick = function (e) {
    if (e.target.nodeName === "LI") {
      document.querySelector(
        UIctrl.getUISelectors().suggestFlag
      ).style.display = "none";

      loadJSONData().then((data) => {
        const country = data.data.filter((item) => {
          return item["name"]["common"] == e.target.textContent;
        });

        const result = document.querySelector(UIctrl.getUISelectors().resultFlag);

        const copyBtn = document.querySelector(UIctrl.getUISelectors().copyBtn);
        
        result.style.display = "block";
        copyBtn.style.display = "block";

        result.innerHTML = JSON.stringify(country, undefined, 2);
      });
    }
  };

  const textCopyClick = function () {
    const result = document.querySelector(UIctrl.getUISelectors().resultFlag);
    if (result.innerHTML != "") {
      let range = document.createRange();
      range.selectNode(result);
      window.getSelection().removeAllRanges();
      window.getSelection().addRange(range);
      document.execCommand("copy");
      window.getSelection().removeAllRanges();
      alert("copied to clipboard");
    }
  };

  return {
    init: function () {
      loadEventListeners();
    },
  };
})(UIctrl);

APPctrl.init();